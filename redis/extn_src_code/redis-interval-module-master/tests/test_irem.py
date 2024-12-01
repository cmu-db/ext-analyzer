from RLTest import Env


class TestIRem():

    def __init__(self):
        self.env = Env()

    def test_item_should_return_error_when_missing_key(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('irem').error()

    def test_item_should_return_error_when_missing_key_member(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('irem', 'intervals').error()

    def test_irem_should_return_0_when_key_is_empty(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('irem', 'intervals', 'i2').equal(0)

    def test_irem_should_return_0_when_member_not_exists(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('irem', 'intervals', 'i2').equal(0)

    def test_irem_should_return_1(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('irem', 'intervals', 'i1').equal(1)

    def test_irem_should_delete_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('irem', 'intervals', 'i1')
        self.env.expect('icard', 'intervals').equal(0)

    def test_irem_should_not_delete_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('irem', 'intervals', 'i2')
        self.env.expect('icard', 'intervals').equal(1)

    def test_irem_should_return_2_when_deleting_2_members(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('irem', 'intervals', 'i1', 'i2').equal(2)

    def test_irem_should_return_1_when_deleting_2_members_but_one_not_exists(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('irem', 'intervals', 'i1', 'i3').equal(1)

    def test_irem_should_return_0_when_deleting_2_members_that_not_exists(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '2,3', 'i2')
        self.env.expect('irem', 'intervals', 'i3', 'i4').equal(0)
