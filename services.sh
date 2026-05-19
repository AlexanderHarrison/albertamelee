#!/bin/bash

cp server.service /etc/systemd/system/
cp get_ab_tournaments.service /etc/systemd/system/
cp get_ab_tournaments.timer /etc/systemd/system/
systemctl reenable server.service
systemctl restart server.service
systemctl reenable get_ab_tournaments.service
systemctl reenable get_ab_tournaments.timer
systemctl restart get_ab_tournaments.service
systemctl restart get_ab_tournaments.timer
