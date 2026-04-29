#!/bin/bash

cp server.service /etc/systemd/system/
systemctl reenable server.service
systemctl restart server.service
