class Hasher:
  ASCII_RANGE = 94

  def fit_digs(self) -> None:
    mask = 1 << self.bits
    print(hex(mask))

    # stage 1
    self.digs   = 1
    self.res = 0

    while self.res < mask:
      self.digs *= 10
      self.total_int()

    if self.digs > 10:
      self.digs = self.digs // 10
      self.total().as_hex()

    # stage 2
    digs_str = str(self.digs)
    len_dstr = len(digs_str)
    stri     = 1
    dig_ret  = str()

    while not (dstr > len_dstr):
      dig = int(digs_str[stri-1])
      dig += 1
      digs_str = digs_str[0:(stri-1)] + str(dig) + digs_str[stri:]
      self.digs = int(digs_str)

      if dig > 9:
        dstr += 1
        continue

      self.digs = int()
      self.total().as_hex()
      if self.res > mask:
        dig -= 1
        dig_ret += str(dig)

      len_dstr -= 1

  def total_int(self) -> int:
    self.res = (self.ASCII_RANGE*self.digs*self.prime*(self.prime+1)) // 2

  def total(self) -> 'Hasher':
    self.res = (self.ASCII_RANGE*self.digs*self.prime*(self.prime+1)) // 2
    return self

  def total_new_digs(self, digs: int) -> 'Hasher':
    self.digs = digs
    return self.total()

  def total_new_prime(self, prime: int) -> 'Hasher':
    self.prime = prime
    return self.total()

  def as_hex(self) -> str:
    print(hex(self.res))
    return hex(self.res)

  def as_int(self) -> None:
    print(self.res)

  def set(self, name: str, val) -> None:
    if type(name) is not str:
      return
    self.__setattr__(name, val)

  def __init__(self, bits: int, digs: int, prime: int):
    self.bits  = bits
    self.digs  = digs
    self.prime = prime
    self.res   = 0

def max_digits_for_prime_at_bits(prime: int, bits: int) -> int:
  mask   = 1 << (bits + 1)
  prime += 1
  d      = 1

  while True:
    fac   = 94*d
    total = sum([fac*i for i in range(1, prime)])

    if total > mask:
      while True:
        fac   = 94*d
        total = sum([fac*i for i in range(1, prime)])
        if total <= mask:
          break
        else:
          d -= 1
      break
    else:
      old_total = total
      d *= 100

  return (d-1) if d > 1 else d

def bytes(hexstr: str) -> int:
  return (len(hexstr) - 2) / 2
