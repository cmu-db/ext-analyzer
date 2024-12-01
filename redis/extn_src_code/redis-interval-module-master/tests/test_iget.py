from RLTest import Env


class TestIGet():

    def __init__(self):
        self.env = Env()

    def test_iget_should_return_error_when_missing_key(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iget').error()

    def test_iget_should_return_error_when_missing_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iget', 'intervals').error()

    def test_iget_should_return_empty_when_member_not_exists(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('iget', 'intervals', 'i1').equal([])

    def test_iget_should_return_i1(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        res = self.env.cmd('iget', 'intervals', 'i1')
        self.env.assertEqual(len(res), 1)
        self.env.assertContains([b'i1', b'1', b'0', b'1', b'1'], res)

    def test_iget_should_return_i2(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        res = self.env.cmd('iget', 'intervals', 'i2')
        self.env.assertEqual(len(res), 1)
        self.env.assertContains([b'i2', b'1', b'2', b'1', b'3'], res)

    def test_iget_should_return_i1_and_i2(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        res = self.env.cmd('iget', 'intervals', 'i1', 'i2')
        self.env.assertEqual(len(res), 2)
        self.env.assertContains([b'i1', b'1', b'0', b'1', b'1'], res)
        self.env.assertContains([b'i2', b'1', b'2', b'1', b'3'], res)

    def test_iget_should_return_only_existing_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('iget', 'intervals', 'i1', 'i3').equal([[b'i1', b'1', b'0', b'1', b'1']])

    def test_iget_should_return_empty_because_no_members_exists(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('iget', 'intervals', 'i3', 'i4').equal([])
