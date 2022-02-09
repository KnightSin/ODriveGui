@echo off

mkdir modules\BatteryEngine
cd modules\BatteryEngine

git restore *
git pull origin master

Pause
