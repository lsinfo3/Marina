import binascii
import hashlib
import random

from crcmod.predefined import mkCrcFun


def jenkins(key, num):
    hash = num
    for i in key:
        hash += i
        hash += hash << 10
        hash ^= hash >> 6

    hash += hash << 3
    hash ^= hash >> 11
    hash += hash << 15
    return hash


crc32 = mkCrcFun('crc32')
crc32_mpeg = mkCrcFun('crc32-mpeg')
crc32_c = mkCrcFun('crc-32c')
crc32_d = mkCrcFun('crc-32d')
crc32_q = mkCrcFun('crc-32q')


class Bloomfilter:
    def __init__(self, bits, num_hashes, hashfunc='crc'):
        self.bits = bits
        self.num_hashes = num_hashes
        self.hashfunc = hashfunc

        self.const_12 = [rand_element(12) for i in range(num_hashes)]

        self.mod = 2 ** bits

        self.bloom = [False] * self.mod

    def hash(self, num: int, data: bytes) -> int:
        if self.hashfunc == 'crc':
            return binascii.crc32(data + bytearray((num,))) % self.mod
        elif self.hashfunc == 'sha1':
            return int.from_bytes(hashlib.sha1(data + bytearray((num,))).digest()[-6:],
                                  "little") % self.mod
        elif self.hashfunc == 'dcrc':
            return binascii.crc32(int(binascii.crc32(data) + num).to_bytes(4, "little")) % self.mod
        elif self.hashfunc == 'xcrc':
            return (binascii.crc32(data) ^ int(f"0x{num}{num}{num}{num}", 16)) % self.mod
        elif self.hashfunc == 'xor':
            return (2 ** 24 * (data[0] ^ data[2] ^ data[4] ^ data[6] ^ data[8]) +
                    2 ** 16 * (data[1] ^ data[3] ^ data[5] ^ data[7] ^ data[9]) +
                    2 ** 8 * (data[2] ^ data[4] ^ data[6] ^ data[8] ^ data[10] ^ num) +
                    1 * (data[3] ^ data[5] ^ data[7] ^ data[9] ^ data[11] ^ num)) % self.mod
        elif self.hashfunc == 'ccrc':
            return binascii.crc32(
                bytearray(data[i] ^ self.const_12[num][i] for i in range(len(data)))) % self.mod
        elif self.hashfunc == 'mccrc':
            res = binascii.crc32(
                bytearray(data[i] ^ self.const_12[num][i] for i in range(len(data))))
            return (res % self.mod) ^ ((res >> 16) % self.mod)
        elif self.hashfunc == 'jenkins':
            return jenkins(data, num) % self.mod
        elif self.hashfunc == 'kmcrc':  # Kirsch-Mitzenmacher Optimization
            return (crc32(data) + num * crc32_mpeg(data)) % self.mod
        elif self.hashfunc == 'kmp4':
            if num == 0:
                res = crc32(data)
            else:
                res = crc32(data) + (crc32_mpeg(data) << num)
            return res % self.mod
        elif self.hashfunc == 'different':
            return [crc32_q, crc32_mpeg, crc32_c, crc32_d][num](data) % self.mod

    def insert(self, data):
        for i in range(self.num_hashes):
            self.bloom[self.hash(i, data)] = True

    def contains(self, data):
        return all([self.bloom[self.hash(i, data)] for i in range(self.num_hashes)])


class SplitBloomfilter(Bloomfilter):
    def __init__(self, bits, num_hashes, hashfunc):
        super().__init__(bits, num_hashes, hashfunc)
        self.bloom = [[False] * self.mod for i in range(num_hashes)]

    def insert(self, data):
        for i in range(self.num_hashes):
            self.bloom[i][self.hash(i, data)] = True

    def contains(self, data):
        return all([self.bloom[i][self.hash(i, data)] for i in range(self.num_hashes)])


def rand_element(length: int):
    return bytearray(random.choices(range(256), k=length))


def create_bloom(bits, num_hashes, elements, length, hashfunc='crc'):
    print(f'{bits}b, {num_hashes}h, {hashfunc} with {elements}')
    b = Bloomfilter(bits, num_hashes, hashfunc)
    s = SplitBloomfilter(bits, num_hashes, hashfunc)
    for i in range(elements):
        el = rand_element(length)
        b.insert(el)
        s.insert(el)
    return b, s


def test_bloom(bloomfilter, s, test_elements, length):
    b_positive = 0
    s_positive = 0
    for i in range(test_elements):
        el = rand_element(length)
        if bloomfilter.contains(el):
            b_positive += 1
        if s.contains(el):
            s_positive += 1
    return b_positive / test_elements, s_positive / test_elements


random.seed(42)
b, s = create_bloom(20, 4, 500_000, 12, 'different')
print("testing...")
print(test_bloom(b, s, 1_00_000, 12))
