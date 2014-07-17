#!/bin/bash
cd friendly-logs
zip -r ../friendly-logs.zip .
cd ../
sudo /opt/psa/bin/extension -i ./friendly-logs.zip
