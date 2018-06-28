#!/bin/bash -eu

################
## User Variables
################

[ $# != 1 ] && echo "error usage = '$0 <Number of Processing Elements>'" && exit 1

SSHKEYFILE=.id_rsa_preesm
CONFIG_FILE=socketcom.conf
NB_PE=$1

[ ! -e $CONFIG_FILE ] && echo "127.0.0.1:25400" > $CONFIG_FILE

################
## Functions
################

function select_exec() {
  DEST_IP=$1
  if [ $(isIpLocal $DEST_IP) == 1 ]; then
    echo "****** Local exec of '${@:2}'"
    ${@:2}
  else
    echo "****** remote ($DEST_IP) exec of '${@:2}'"
    remote_exec $@
  fi
}

function remote_exec() {
  IP=$1
  USR=$(cat $CONFIG_FILE | grep $IP | cut -d':' -f3 | sort -u | head -n 1)
  COMMAND="${@:2}"
  
  # create tmp files on both sides
  LOCALTMP=$(mktemp)
  REMOTETMP=$(ssh -i ${SSHKEYFILE} ${USR}@${IP} /bin/bash -c "mktemp")
  
  # init local script then transfer it
  echo "$COMMAND" > ${LOCALTMP}
  scp -q -i ${SSHKEYFILE} ${LOCALTMP} ${USR}@${IP}:${REMOTETMP}
  ssh -i ${SSHKEYFILE} ${USR}@${IP} chmod 777 ${REMOTETMP}
  
  # exec script
  echo "[REMOTE - START] $COMMAND"
  set +e
  ssh -i ${SSHKEYFILE} ${USR}@${IP} /bin/bash -c ${REMOTETMP}
  RES=$?
  set -e
  echo "[REMOTE - EXIT (${RES})] $COMMAND"
  
  # cleanup
  ssh -i ${SSHKEYFILE} ${USR}@${IP} rm ${REMOTETMP}
  rm ${LOCALTMP}
  
  return ${RES}
}

function isIpLocal() {
  ARG_IP=$1
  LOCAL_IPS=$(hostname -I)
  IP_IS_SAME=$(echo $ARG_IP | sed 's/\(127\..*\)/\1/g' | wc -l)
  if [ "$IP_IS_SAME" == "0" ]; then
    for LOCAL_IP in $LOCAL_IPS; do
      if [ "$LOCAL_IP" == "$ARG_IP" ]; then
        IP_IS_SAME=1
        break;
      fi
    done
  fi
  echo $IP_IS_SAME
}

function select_cp() {
  DEST_IP=$1
  SRC_FILE=$2
  DST_FOLDER=$3
  if [ $(isIpLocal $DEST_IP) == 1 ]; then
    cp ${@:2}
  else
    USR=$(cat $CONFIG_FILE | grep $IP | cut -d':' -f3 | sort -u | head -n 1)
    scp -q -i ${SSHKEYFILE} ${SRC_FILE}/ ${USR}@${DEST_IP}:${DST_FOLDER}
  fi
}

################
## Configuration of SSH for non interactive remote actions
################

MAIN_PE_HOST=$(cat $CONFIG_FILE | head -n 1 | cut -d':' -f1 | xargs)
PE_HOST_LIST=$(cat $CONFIG_FILE | tail -n +2 | cut -d':' -f2 | grep -v ${MAIN_PE_HOST} | sort -u | xargs)

# generate dedicated private key
[ ! -e ${SSHKEYFILE} ] && ssh-keygen -f ${SSHKEYFILE} -t rsa -N ''  

# setup public SSH key on remote hosts (may require password)
for IP in $PE_HOST_LIST $MAIN_PE_HOST; do
  USR=$(cat $CONFIG_FILE | grep $IP | cut -d':' -f3 | sort -u | head -n 1)
  if [ $(isIpLocal $IP) == 1 ]; then
    echo "****** Skip  [$USR@$IP] -- IP is local"
  else
    echo "****** Setup [$USR@$IP]"
    ssh-copy-id -i ${SSHKEYFILE}.pub ${USR}@${IP}
  fi
done

################
## Send source code to all hosts and compile it
################

LOCAL_TMP_DIR=$(mktemp -d)
##
## XXX : assumes the script is located in the generated folder
##
SCRIPT_DIR=$(cd $(dirname $0) && pwd)
#CODE_DIR=$(cd ${SCRIPT_DIR}/.. && pwd)
CODE_DIR=$(cd ${SCRIPT_DIR} && pwd)

(cd ${CODE_DIR} && tar cjf ${LOCAL_TMP_DIR}/code.tar.bz2 *)

for IP in $PE_HOST_LIST $MAIN_PE_HOST; do
  select_exec $IP rm -rf /tmp/preesm_tcp_exec
  select_exec $IP mkdir -p /tmp/preesm_tcp_exec
  select_cp $IP ${LOCAL_TMP_DIR}/code.tar.bz2 /tmp/preesm_tcp_exec
  select_exec $IP tar xf /tmp/preesm_tcp_exec/code.tar.bz2 -C /tmp/preesm_tcp_exec/
  select_exec $IP chmod +x /tmp/preesm_tcp_exec/build.sh
  select_exec $IP /tmp/preesm_tcp_exec/build.sh
done

################
## Run binary with appropriate IDs
################

IP=$(cat $CONFIG_FILE | head -n 1 | cut -d':' -f1 | xargs)
(select_exec $IP /tmp/preesm_tcp_exec/run.sh 0) &

for i in $(seq 2 $NB_PE); do
  ID=$((i - 1))
  set +e
  NB=$(cat $CONFIG_FILE | grep -c "^$ID:")
  set -e
  if [ "$NB" == "0" ]; then
    IP=$MAIN_PE_HOST
  else
    IP=$(cat $CONFIG_FILE | grep "^$ID:" | cut -d':' -f2 | xargs)
  fi
  (select_exec $IP /tmp/preesm_tcp_exec/run.sh $ID) &
done
echo waitin...
# wait end of exec
for i in $(seq 0 $((NB_PE - 1))); do
  wait
done

