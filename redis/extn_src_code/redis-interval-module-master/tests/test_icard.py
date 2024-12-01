from RLTest import Env


class TestICard():

    def __init__(self):
        self.env = Env()

    def test_icard_with_0_intervals_should_return_0(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icard', 'intervals').equal(0)

    def test_icard_should_return_1(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('icard', 'intervals').equal(1)

    def test_icard_should_return_0_on_other_key(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('icard', 'other').equal(0)

    def test_icard_should_return_2(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('icard', 'intervals').equal(2)
