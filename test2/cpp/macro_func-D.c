// RUN: %ucc '-DF(x)=x+1' -E %s|grep -F '2+1'

F(2)
