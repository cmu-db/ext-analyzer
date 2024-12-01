from RLTest import Env


class TestIAdd():

    def __init__(self):
        self.env = Env()

    def test_iadd_with_0_arguments_should_return_an_error(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd').error()

    def test_iadd_with_1_arguments_should_return_an_error(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals').error()

    def test_iadd_with_2_arguments_should_return_an_error(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', '0,1').error()

    def test_iadd_with_3_arguments_should_return_an_error(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', '0,1').error()

    def test_iadd_with_1_interval_should_return_1(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', '0,1', 'i1').equal(1)

    def test_iadd_with_2_intervals_should_return_2(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', '0,1', 'i1', '2,3', 'i2').equal(2)
