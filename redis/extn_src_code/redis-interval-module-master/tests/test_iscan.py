from RLTest import Env


class TestIScan():

    def __init__(self):
        self.env = Env()

    def test_iscan_should_return_an_error_with_0_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iscan').error()

    def test_iscan_should_return_an_error_with_1_parameter(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iscan', 'intervals').error()

    def test_iscan_should_return_an_error_with_2_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iscan', 'intervals', '0').error()

    def test_iscan_should_return_an_error_with_3_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iscan', 'intervals', '0', 'MATCH').error()

    def test_iscan_should_return_an_error_when_match_is_missing(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('iscan', 'intervals', '0', 'COUNT', '*').error()

    def test_iscan_should_return_0_scan_is_finished(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*').contains(0)

    def test_iscan_should_return_greater_than_0_when_scan_is_not_finished(self):
        self.env.cmd('FLUSHALL')
        for i in range(100):
            self.env.cmd('iadd', 'intervals', '0,' + str(i), 'i' + str(i))
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*').notContains(0)

    def test_iscan_should_return_i1(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*').equal([0, [[b'i1', b'1', b'0', b'1', b'1']]])

    def test_iscan_should_return_i1_and_i2(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*').equal(
            [0, [[b'i1', b'1', b'0', b'1', b'1'], [b'i2', b'1', b'2', b'1', b'3']]])

    def test_iscan_should_return_only_one_member(self):
        self.env.cmd('FLUSHALL')
        for i in range(100):
            self.env.cmd('iadd', 'intervals', '0,' + str(i), 'i' + str(i))
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*', 'COUNT', '1').equal(
            [73, [[b'i10', b'1', b'0', b'1', b'10']]]
            )

    def test_iscan_should_return_only_two_members(self):
        self.env.cmd('FLUSHALL')
        for i in range(100):
            self.env.cmd('iadd', 'intervals', '0,' + str(i), 'i' + str(i))
        self.env.expect('iscan', 'intervals', '0', 'MATCH', '*', 'COUNT', '2').equal(
            [74, [[b'i10', b'1', b'0', b'1', b'10'], [b'i11', b'1', b'0', b'1', b'11']]]
            )

    def test_iscan_should_return_only_i22(self):
        self.env.cmd('FLUSHALL')
        for i in range(100):
            self.env.cmd('iadd', 'intervals', '0,' + str(i), 'i' + str(i))
        self.env.expect('iscan', 'intervals', '0', 'MATCH', 'i22', 'COUNT', '1000000').equal(
            [0, [[b'i22', b'1', b'0', b'1', b'22']]]
            )
