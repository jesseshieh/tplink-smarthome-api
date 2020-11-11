#!/usr/bin/env bash

read MESSAGE

MSG=$(echo "${MESSAGE}" | tr -d '\n')

HOST="10.0.1.39"

if [ $MSG = "state" ]; then
  # remove colors and pull out relay_state field
  tplink-smarthome-api getSysInfo $HOST | sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" | gawk 'match($0, /\s*relay_state:\s+([10]),/, m) {print m[1]}'
else
  tplink-smarthome-api setPowerState $HOST $MSG
fi
