from os import system

system(r"grep -oe '\[[0-9]\+\] ' /home/p/vsl/primtab.c | sed -Ee 's:\[(.*)\] :\1:' > /tmp/numbers.txt")

with open("/tmp/numbers.txt") as f:
  numbers = [int(line.rstrip()) for line in f]
  f.close()

nlen = len(numbers)
avg  = 0

for i in range(1, nlen):
  avg += numbers[i] - numbers[i-1]

avg = avg / (nlen - 1)

print(avg)
