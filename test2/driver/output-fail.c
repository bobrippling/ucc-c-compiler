// RUN: %ucc -c -o /doesnotexist/somename %s; [ $? -ne 0 ]
