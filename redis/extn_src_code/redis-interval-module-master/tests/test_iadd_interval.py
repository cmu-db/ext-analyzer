from RLTest import Env

maxBound = '99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999' \
           '99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999' \
           '99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999' \
           '99999999999999999999999'

class TestIAdd():

    def __init__(self):
        self.env = Env()

    def shouldError(self, interval):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', interval, 'i1').error()

    def shouldNoError(self, interval):
        self.env.cmd('FLUSHALL')
        self.env.expect('iadd', 'intervals', interval, 'i1').equal(1)

    def expectInterval(self, interval, expected):
        self.env.cmd('FLUSHALL')
        self.env.cmd('iadd', 'intervals', interval, 'i1')
        self.env.expect('iscan', 'intervals', '0', 'MATCH', 'i1', 'COUNT', '9999').equal([0, [expected]])

    def test_iadd_empty_interval_should_return_an_error(self):
        self.shouldError('')

    def test_iadd_interval_with_only_bound_separator_should_return_an_error(self):
        self.shouldError(',')

    def test_iadd_interval_with_only_1_number_should_return_an_error(self):
        self.shouldError('0')

    def test_iadd_interval_with_only_lower_bound_should_return_an_error(self):
        self.shouldError('0,')

    def test_iadd_interval_with_only_upper_bound_should_return_an_error(self):
        self.shouldError(',1')

    def test_iadd_interval_with_text_should_return_an_error(self):
        self.shouldError('test')

    def test_iadd_interval_with_text_on_lower_bound_should_return_an_error(self):
        self.shouldError('test,1')

    def test_iadd_interval_with_text_on_upper_bound_should_return_an_error(self):
        self.shouldError('0,test')

    def test_iadd_interval_with_lower_greater_than_upper_should_return_an_error(self):
        self.shouldError('1,0')

    def test_iadd_interval_with_minus_inf_on_upper_should_return_an_error(self):
        self.shouldError('1,-inf')

    def test_iadd_interval_with_inf_on_lower_should_return_an_error(self):
        self.shouldError('inf,1')

    def test_iadd_interval_with_plus_inf_on_lower_should_return_an_error(self):
        self.shouldError('+inf,1')

    def test_iadd_interval_with_inf_reversed_should_return_an_error(self):
        self.shouldError('+inf,-inf')

    def test_iadd_interval_with_invalid_bound_inclusion_should_return_an_error(self):
        self.shouldError('(+inf,-inf}')

    def test_iadd_with_valid_interval_should_return_1(self):
        self.shouldNoError('0,1')

    def test_iadd_with_minus_inf_on_lower_interval_should_return_1(self):
        self.shouldNoError('-inf,1')

    def test_iadd_with_inf_on_upper_interval_should_return_1(self):
        self.shouldNoError('0,inf')

    def test_iadd_with_plus_inf_on_upper_interval_should_return_1(self):
        self.shouldNoError('0,+inf')

    def test_iadd_with_valid_bounds_1_should_return_1(self):
        self.shouldNoError('[0,1]')

    def test_iadd_with_valid_bounds_2_should_return_1(self):
        self.shouldNoError('[0,1[')

    def test_iadd_with_valid_bounds_3_should_return_1(self):
        self.shouldNoError(']0,1]')

    def test_iadd_with_valid_bounds_4_should_return_1(self):
        self.shouldNoError(']0,1[')

    def test_iadd_with_valid_bounds_with_decimal_number(self):
        self.shouldNoError(']0.123456789,1.987654321[')

    def test_iadd_with_valid_bounds_with_huge_decimal_number(self):
        self.shouldNoError(']-9.99999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '9999999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '9999999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '999999999999999999999999999999999999999999999999999999999999999999999999'
                           ','
                           '9.99999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '9999999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '9999999999999999999999999999999999999999999999999999999999999999999999999999999'
                           '999999999999999999999999999999999999999999999999999999999999999999999999[')

    def test_iadd_with_valid_hightest_min_bound_should_not_return_an_error(self):
        self.shouldNoError('-' + maxBound + ',1')

    def test_iadd_with_valid_hightest_max_bound_should_not_return_an_error(self):
        self.shouldNoError('0,' + maxBound)

    def test_iadd_with_valid_hightest_min_bound_and_intinity_should_not_return_an_error(self):
        self.shouldNoError(maxBound + ',+inf')

    def test_iadd_with_valid_hightest_max_bound_and_infinity_should_not_return_an_error(self):
        self.shouldNoError('-inf,-' + maxBound)

    def test_iadd_with_greater_number_on_max_bound_should_not_return_an_error(self):
        self.shouldNoError('-inf,' + maxBound + '9')

    def test_iadd_with_greater_number_on_min_bound_should_not_return_an_error(self):
        self.shouldNoError('-' + maxBound + '9,+inf')

    def test_iadd_should_return_minus_inf_plus_inf_when_upper_overflow(self):
        self.expectInterval('-inf,' + maxBound + '9', [b'i1', b'1', b'-inf', b'1', b'+inf'])

    def test_iadd_should_return_minus_inf_plus_inf_when_lower_overflow(self):
        self.expectInterval('-' + maxBound + '9,+inf', [b'i1', b'1', b'-inf', b'1', b'+inf'])
