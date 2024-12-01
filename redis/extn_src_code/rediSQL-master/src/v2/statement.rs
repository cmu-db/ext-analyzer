use parser::common::CommandV2;
use parser::statement::Action;
use parser::statement::Statement;

use redisql_lib::redis as r;
use redisql_lib::redis::LoopData;
use redisql_lib::redis::RedisReply;
use redisql_lib::redis::ReturnMethod;
use redisql_lib::redis::Returner;
use redisql_lib::redis::StatementCache;
use redisql_lib::redis_type::BlockedClient;
use redisql_lib::redis_type::ReplicateVerbatim;
use redisql_lib::sqlite::QueryResult;

use crate::common::{free_privdata, reply_v2, timeout};

#[allow(non_snake_case)]
pub extern "C" fn Statement_v2(
    ctx: *mut r::rm::ffi::RedisModuleCtx,
    argv: *mut *mut r::rm::ffi::RedisModuleString,
    argc: ::std::os::raw::c_int,
) -> i32 {
    let context = r::rm::Context::new(ctx);
    let argvector = match r::create_argument(argv, argc) {
        Ok(argvector) => argvector,
        Err(mut error) => {
            return error.reply_v2(&context);
        }
    };
    let command: Statement = match CommandV2::parse(argvector) {
        Ok(comm) => comm,
        Err(mut e) => return e.reply_v2(&context),
    };
    let key = command.key(&context);
    if !command.is_now() {
        match key.get_channel() {
            Err(mut e) => e.reply_v2(&context),
            Ok(ch) => {
                let blocked_client = BlockedClient::new(
                    &context,
                    reply_v2,
                    timeout,
                    free_privdata,
                    10_000,
                );
                let command = command.get_command(blocked_client);
                match ch.send(command) {
                    Err(e) => {
                        dbg!(
                            "Error in sending the command!",
                            e.to_string()
                        );
                        r::rm::ffi::REDISMODULE_OK
                    }
                    _ => r::rm::ffi::REDISMODULE_OK,
                }
            }
        }
    } else {
        let loop_data = match key.get_loop_data() {
            Ok(k) => k,
            Err(mut e) => return e.reply_v2(&context),
        };
        match command.get_action() {
            Action::New => {
                let result = loop_data
                    .get_replication_book()
                    .insert_new_statement(
                        command.identifier(),
                        command.statement(),
                        command.can_update(),
                    );
                match result {
                    Err(mut e) => e.reply_v2(&context),
                    Ok(_) => {
                        ReplicateVerbatim(&context);
                        (QueryResult::OK {}).reply_v2(&context)
                    }
                }
            }
            Action::Update => {
                let result = loop_data
                    .get_replication_book()
                    .update_statement(
                        command.identifier(),
                        command.statement(),
                        command.can_create(),
                    );
                match result {
                    Err(mut e) => e.reply_v2(&context),
                    Ok(_) => {
                        ReplicateVerbatim(&context);
                        (QueryResult::OK {}).reply_v2(&context)
                    }
                }
            }

            Action::Delete => {
                let result = loop_data
                    .get_replication_book()
                    .delete_statement(command.identifier());
                match result {
                    Err(mut e) => e.reply_v2(&context),
                    Ok(_) => {
                        ReplicateVerbatim(&context);
                        (QueryResult::OK {}).reply_v2(&context)
                    }
                }
            }
            Action::List => {
                let result = loop_data
                    .get_replication_book()
                    .list_statements();
                match result {
                    Err(mut e) => e.reply_v2(&context),
                    Ok(q) => {
                        let mut to_return = q.create_data_to_return(
                            &context,
                            &ReturnMethod::ReplyWithHeader,
                            std::time::Instant::now()
                                + std::time::Duration::from_secs(10),
                        );
                        to_return.reply_v2(&context)
                    }
                }
            }
            _ => todo!(),
        }
    }
}
