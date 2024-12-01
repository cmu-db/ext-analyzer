/**************************************************************************
   Copyright (c) 2022 sewenew

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#include <sw/redis++/redis++.h>
#include <iostream>
#include "append_test.h"
#include "clear_test.h"
#include "del_test.h"
#include "type_test.h"
#include "schema_test.h"
#include "set_get_test.h"
#include "len_test.h"
#include "merge_test.h"
#include "import_test.h"

int main() {
    try {
        auto r = sw::redis::Redis("tcp://127.0.0.1");

        sw::redis::pb::test::AppendTest append_test(r);
        append_test.run();

        sw::redis::pb::test::ClearTest clear_test(r);
        clear_test.run();

        sw::redis::pb::test::DelTest del_test(r);
        del_test.run();

        sw::redis::pb::test::TypeTest type_test(r);
        type_test.run();

        sw::redis::pb::test::SchemaTest schema_test(r);
        schema_test.run();

        sw::redis::pb::test::SetGetTest set_get_test(r);
        set_get_test.run();

        sw::redis::pb::test::LenTest len_test(r);
        len_test.run();

        sw::redis::pb::test::MergeTest merge_test(r);
        merge_test.run();

        sw::redis::pb::test::ImportTest import_test(r);
        import_test.run();

        std::cout << "pass all tests" << std::endl;
    } catch (const sw::redis::Error &e) {
        std::cerr << "failed to do test: " << e.what() << std::endl;
    }

    return 0;
}
