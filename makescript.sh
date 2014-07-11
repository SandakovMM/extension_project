#!/bin/bash
cd friendly-logs
zip -r ../friendly-logs.zip .
cd ../
/opt/psa/bin/extension -i ./friendly-logs.zip
