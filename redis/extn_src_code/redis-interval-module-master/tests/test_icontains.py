from RLTest import Env


class TestIContains():

    def __init__(self):
        self.env = Env()

    def test_icontains_should_return_an_error_with_0_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains').error()

    def test_icontains_should_return_an_error_with_1_parameter(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals').error()

    def test_icontains_should_return_an_error_when_second_parameter_is_not_a_number(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals', 'test').error()

    def test_icontains_should_return_an_error_when_count_is_incomplete(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals', '1', 'COUNT').error()

    def test_icontains_should_return_an_error_when_count_amount_is_negative(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals', '1', 'COUNT', '-1').error()

    def test_icontains_should_return_an_error_when_count_amount_is_not_a_number(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals', '1', 'COUNT', 'test').error()

    def test_icontains_should_return_empty_when_key_is_empty(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('icontains', 'intervals', '0').equal([])

    def test_icontains_should_return_one_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('icontains', 'intervals', '0').equal([[b'i1', b'1', b'0', b'1', b'1']])

    def test_icontains_should_return_empty_when_value_is_out_of_interval(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.expect('icontains', 'intervals', '2').equal([])

    def test_icontains_should_return_empty_when_value_is_equals_to_upper_and_upper_is_excluded(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1[', 'i1')
        self.env.expect('icontains', 'intervals', '1').equal([])

    def test_icontains_should_return_empty_when_value_is_equals_to_lower_and_lower_is_excluded(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', ']0,1', 'i1')
        self.env.expect('icontains', 'intervals', '0').equal([])

    def test_icontains_should_return_2_members(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,3', 'i1')
        self.env.cmd('iadd', 'intervals', '0,5', 'i2')
        res = self.env.cmd('icontains', 'intervals', '2')
        self.env.assertEqual(len(res), 2)
        self.env.assertContains([b'i2', b'1', b'0', b'1', b'5'], res)
        self.env.assertContains([b'i1', b'1', b'0', b'1', b'3'], res)

    def test_icontains_should_return_one_interval(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,10', 'i1')
        self.env.cmd('iadd', 'intervals', '5,15', 'i2')
        res = self.env.cmd('icontains', 'intervals', '7', 'COUNT', '1')
        self.env.assertEqual(len(res), 1)

    def test_icontains_should_return_two_intervals(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,10', 'i1')
        self.env.cmd('iadd', 'intervals', '5,15', 'i2')
        self.env.cmd('iadd', 'intervals', '5,20', 'i3')
        res = self.env.cmd('icontains', 'intervals', '7', 'COUNT', '2')
        self.env.assertEqual(len(res), 2)

    def test_icontains_should_store_result_in_an_other_key(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '0,2', 'i2')
        self.env.cmd('iadd', 'intervals', '0,3', 'i3')
        self.env.cmd('iadd', 'intervals', '0,4', 'i4')
        self.env.cmd('icontains', 'intervals', '3', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 2)

    def test_icontains_should_store_result_in_an_other_key_only_1_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '0,2', 'i2')
        self.env.cmd('iadd', 'intervals', '0,3', 'i3')
        self.env.cmd('iadd', 'intervals', '0,4', 'i4')
        self.env.cmd('icontains', 'intervals', '3', 'COUNT', '1', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 1)

    def test_icontains_should_return_an_error_if_store_key_is_not_an_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('set', 'foo', 'bar')
        self.env.expect('icontains', 'intervals', '3', 'COUNT', '1', 'STORE', 'foo').error()

    def test_icontains_should_store_result_in_an_other_not_empty_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'destination', '2,3', 'i2')
        self.env.cmd('icontains', 'intervals', '1', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 2)

    def test_icontains_should_update_key_in_other_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'destination', '2,3', 'i1')
        self.env.expect('icontains', 'intervals', '1', 'STORE', 'destination').equal(1)
        res = self.env.cmd('iget', 'destination', 'i1')
        self.env.assertEqual(res, [[b'i1', b'1', b'0', b'1', b'1']])
