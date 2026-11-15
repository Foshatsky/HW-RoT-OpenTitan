import random
import cocotb
from cocotb.triggers import Timer


@cocotb.test()
async def test_adder_constrained_random(dut):
    random.seed(42)
    for i in range(100):
        a = random.randint(0, 15)
        b = random.randint(0, 15)
        dut.a.value = a
        dut.b.value = b
        await Timer(1, unit="ns")
        expected = a + b
        actual = int(dut.sum.value)
        assert actual == expected, f"FAIL: a={a} b={b} expected={expected} got={actual}"
    dut._log.info("All 100 constrained-random tests passed!")
