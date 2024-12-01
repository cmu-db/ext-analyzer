#!/usr/bin/env bash

set -e

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PID_FILE="${CURRENT_DIR}/integration_test.pid"
LOG_FILE="${CURRENT_DIR}/integration_test.log"

REDIS_CLI_RESULT= # use global variable for function result


# fancy output functions
function with_timestamp () {
  while read -r line; do
    local datestring=`date +'%Y-%m-%d %H:%M:%S'`
    echo -e "${datestring} | ${line}" | tee -a ${LOG_FILE}
  done
}
function echo_message () {
  local MESSAGE="${1}"
  echo "${MESSAGE}" | with_timestamp
}
function info () {
  echo_message "INFO:  ${1}"
}
function error () {
  if [[ "${#}" -ge 2 ]] ; then
    local LINE="on line ${1}: "
    local MESSAGE="${2}"
    local CODE="${3:-1}"
  else
    local LINE=""
    local MESSAGE="${1}"
    local CODE="${2:-1}"
  fi

  if [[ -n "${MESSAGE}" ]] ; then
    echo_message "ERROR: ${LINE}${MESSAGE}; Exit status: ${CODE}"
  else
    echo_message "ERROR: ${LINE}Exit status: ${CODE}"
  fi
  exit "${CODE}"
}
trap 'error ${LINENO}' ERR

# test functions
function redis_cli () {
  local COMMAND="${@}"
  info "> redis-cli ${COMMAND}"

  REDIS_CLI_RESULT=`docker exec redis4_with_fpn redis-cli ${COMMAND}`
  # REDIS_CLI_RESULT=`redis-cli ${COMMAND}`
  info "< '${REDIS_CLI_RESULT}'"
}
function is () {
  local EXPECTED_RESULT="${1}"
  local COMMAND="${@:2}"

  redis_cli ${COMMAND}
  if [ "${REDIS_CLI_RESULT}" == "${EXPECTED_RESULT}" ];
  then
    info "OK"
  else
    error "Expected '${EXPECTED_RESULT}', but was '${REDIS_CLI_RESULT}'"
  fi
}


# svae PID to file
echo "$$" > "${PID_FILE}"
# clear previous log file
echo "" > "${LOG_FILE}"


OUT_OF_BOUND_NUMBER="value must be greater or equal than -99_999_999_999_999_999_999_999_999_999_999_999_999 and lower or equal than 99_999_999_999_999_999_999_999_999_999_999_999_999"


is                                        ""  FPN.GET popa.key
is                                    "0.00"  FPN.SET popa.key 1.23
is                                   "4.797"  FPN.ADD popa.key 3.567
is                                   "4.797"  FPN.GET popa.key
is                                    "4.80"  FPN.GET popa.key 2
is                                       "5"  FPN.GET popa.key 0
is                                   "1.797"  FPN.SUBTRACT popa.key 3
is                                 "4.24092"  FPN.MULTIPLY popa.key 2.36
is                                     "8.5"  FPN.MULTIPLY popa.key 2 1
is                                     "8.5"  FPN.SET popa.key 1234567891234567890.0123456789123456789
is "1234567891234567890.0123456789123456789"  FPN.GET popa.key
is "1234567891234567890.0123456789123456789"  FPN.GET popa.key 19
is                  "${OUT_OF_BOUND_NUMBER}"  FPN.GET popa.key 20
