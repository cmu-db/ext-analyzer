from RLTest import Env


class TestIOverlaps():

    def __init__(self):
        self.env = Env()

    def test_ioverlaps_should_return_an_error_with_0_parameters(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps').error()

    def test_ioverlaps_should_return_an_error_with_1_parameter(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps', 'intervals').error()

    def test_ioverlaps_should_return_an_error_when_count_is_incomplete(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps', 'intervals', '0,1', 'COUNT').error()

    def test_ioverlaps_should_return_an_error_when_count_amount_is_negative(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps', 'intervals', '0,1', 'COUNT', '-1').error()

    def test_ioverlaps_should_return_an_error_when_count_amount_is_not_a_number(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps', 'intervals', '0,1', 'COUNT', 'test').error()

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_is_in_the_interval(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '6,7').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_interval_is_in_the_parameter_interval(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '0,20').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_overlaps_lower_bound(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '0,7').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_overlaps_upper_bound(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '7,15').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_abuts_lower_bound(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '0,5').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_abuts_upper_bound(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '10,15').equal(
            [[b'i1', b'1', b'5', b'1', b'10']])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_abuts_lower_bound_and_lower_not_included(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', ']5,10', 'i1')
        self.env.expect('ioverlaps', 'intervals', '0,5').equal([])

    def test_ioverlaps_should_return_the_interval_when_parameter_interval_abuts_upper_bound_and_upper_not_included(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '5,10[', 'i1')
        self.env.expect('ioverlaps', 'intervals', '10,15').equal([])

    def test_ioverlaps_should_return_an_error_when_interval_is_incorrect(self):
        self.env.cmd('FLUSHALL')
        self.env.expect('ioverlaps', 'intervals', '10,').error()

    def test_ioverlaps_should_return_one_interval(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,10', 'i1')
        self.env.cmd('iadd', 'intervals', '5,15', 'i2')
        res = self.env.cmd('ioverlaps', 'intervals', '7,12', 'COUNT', '1')
        self.env.assertEqual(len(res), 1)

    def test_ioverlaps_should_return_two_intervals(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,10', 'i1')
        self.env.cmd('iadd', 'intervals', '5,15', 'i2')
        self.env.cmd('iadd', 'intervals', '10,20', 'i3')
        res = self.env.cmd('ioverlaps', 'intervals', '7,12', 'COUNT', '2')
        self.env.assertEqual(len(res), 2)


    def test_ioverlaps_should_store_result_in_an_other_key(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '0,2', 'i2')
        self.env.cmd('iadd', 'intervals', '0,3', 'i3')
        self.env.cmd('iadd', 'intervals', '0,4', 'i4')
        self.env.cmd('ioverlaps', 'intervals', '3,4', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 2)

    def test_ioverlaps_should_store_result_in_an_other_key_only_1_member(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'intervals', '0,2', 'i2')
        self.env.cmd('iadd', 'intervals', '0,3', 'i3')
        self.env.cmd('iadd', 'intervals', '0,4', 'i4')
        self.env.cmd('ioverlaps', 'intervals', '3,4', 'COUNT', '1', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 1)

    def test_ioverlaps_should_return_an_error_if_store_key_is_not_an_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('set', 'foo', 'bar')
        self.env.expect('ioverlaps', 'intervals', '3,4', 'COUNT', '1', 'STORE', 'foo').error()

    def test_ioverlaps_should_store_result_in_an_other_not_empty_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'destination', '2,3', 'i2')
        self.env.cmd('ioverlaps', 'intervals', '0,1', 'STORE', 'destination')
        res = self.env.cmd('icard', 'destination')
        self.env.assertEqual(res, 2)

    def test_ioverlaps_should_update_key_in_other_interval_set(self):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', '0,1', 'i1')
        self.env.cmd('iadd', 'destination', '2,3', 'i1')
        self.env.expect('ioverlaps', 'intervals', '0,1', 'STORE', 'destination').equal(1)
        res = self.env.cmd('iget', 'destination', 'i1')
        self.env.assertEqual(res, [[b'i1', b'1', b'0', b'1', b'1']])
