PRIME = 97;
MAX_RANGE = 94;

def s(t: int) -> int:
  return (t*(t+1)//2);

def m(t: int) -> int:
  return t // (PRIME-1);

def n(t: int) -> int:
  return s(t % PRIME);

def sum_for_length(t: int) -> int:
  return MAX_RANGE*(m(t)*s(PRIME-1) + n(t));

def range_for_length(t: int) -> int:
  return (0, sum_for_length(t) - s(t))
