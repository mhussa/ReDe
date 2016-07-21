#include "redismodule.h"
#include "rmutil/util.h"
#include "rmutil/strings.h"
#include "rmutil/test_util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

//Name of a hash-map mapping element IDs to queue timeouts (integers)
#define REDIS_QUEUE_MAP "element_queues"

//Name of a hash-map mapping element IDs to elements (JSON strings)
#define REDIS_ELEMENT_MAP "element_objects"

// Name of a hash-map mapping element IDs to elements (JSON strings)
#define REDIS_EXPIRATION_MAP "element_expiration"

// Format of the queue name (for storage)
#define REDIS_QUEUE_NAME_FORMAT "timeout_queue#%d"
#define REDIS_QUEUE_NAME_FORMAT_PATTERN "timeout_queue#*"


/*
* dehydrator.push <element_id> <element> <timeout>
* dehydrate <element> for <timeout> seconds
*/
int PushCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    // we need EXACTLY 4 arguments  TODO: make sure what is in argv[0]
    if (argc != 4) {
      return RedisModule_WrongArity(ctx);
    }
    RedisModule_AutoMemory(ctx);

    element_id = argv[1]
    element = argv[2]
    timeout = argv[3]


    // make sure we have the queue listed
    RedisModuleCallReply *srep =
        RedisModule_Call(ctx, "HSET", "css", REDIS_QUEUE_MAP, element_id, timeout);
    RMUTIL_ASSERT_NOERROR(srep);

    // add the element to the dehydrating elements map
    RedisModuleCallReply *srep =
        RedisModule_Call(ctx, "HSET", "css", REDIS_ELEMENT_MAP, element_id, element);
    RMUTIL_ASSERT_NOERROR(srep);


    time_t result = time(NULL);
    // if(result != -1) //TODO: this seatbelt may be needed
    expiration = (uintmax_t)result + timeout;

    // add the element to the element expiration hash
    RedisModuleCallReply *srep =
        RedisModule_Call(ctx, "HSETNX", "csl", REDIS_EXPIRATION_MAP, element_id, expiration);
    RMUTIL_ASSERT_NOERROR(srep);


    // add the elemet id to the actual dehydration queue
    char dehydration_queue_name[30];
    sprintf(dehydration_queue_name, REDIS_QUEUE_NAME_FORMAT, timeout);
    RedisModuleCallReply *srep =
        RedisModule_Call(ctx, "RPUSH", "csl", dehydration_queue_name, element_id, expiration);
    RMUTIL_ASSERT_NOERROR(srep);
    free(dehydration_queue_name);

    return 0;
}

/*
* dehydrator.poll 
* get all elements which were dried for long enogh
*/
int PollCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModuleCallReply *rep =
        RedisModule_Call(ctx, "KEYS", "c" ,REDIS_QUEUE_NAME_FORMAT_PATTERN);
    RMUTIL_ASSERT_NOERROR(rep); 
    timeouts = REDISMODULE_REPLY_ARRAY(rep)
    size_t timeout_num = RedisModule_CallReplyLength(rep)




        // print "timeouts: ", timeouts
        while timeouts:
            //Pull next item for all timeouts (effeciently)
            for timeout in timeouts:
                self._pipe.lpop(timeout)
            items = self._pipe.execute()
            // print "items: ", items
            // print "timeouts: ",list(timeouts)
      elements = []
      next_timeouts = set()
            for timeout, element_id in zip(timeouts, items):
                if element_id:
          element = self._inspect(element_id, self._queue_to_int(timeout))
                    if element is not None:
                        # element was rehydrated, return to this queue to see if
                        # there are more rehydratable elements
            elements.append(element)
                        // print "%s returned" % element
                        next_timeouts.add(timeout)
                    else:
                        # this element needs to dehydrate longer, push it back
                        # to the front of the queue
                        // print "%s inspect is false" % element_id
                        self._pipe.lpush(timeout, element_id)
            self._pipe.execute()
            timeouts = next_timeouts

    return elements


  RedisModule_AutoMemory(ctx);


  // open the key and make sure it's indeed a HASH and not empty
  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_HASH &&
      RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  // get the current value of the hash element
  RedisModuleCallReply *rep =
      RedisModule_Call(ctx, "HGET", "ss", argv[1], argv[2]);
  RMUTIL_ASSERT_NOERROR(rep);

  // set the new value of the element
  RedisModuleCallReply *srep =
      RedisModule_Call(ctx, "HSET", "sss", argv[1], argv[2], argv[3]);
  RMUTIL_ASSERT_NOERROR(srep);

  // if the value was null before - we just return null
  if (RedisModule_CallReplyType(rep) == REDISMODULE_REPLY_NULL) {
    RedisModule_ReplyWithNull(ctx);
    return REDISMODULE_OK;
  }

  // forward the HGET reply to the client
  RedisModule_ReplyWithCallReply(ctx, rep);
  return REDISMODULE_OK;
}

// test the HGETSET command
int testHgetSet(RedisModuleCtx *ctx) {
  RedisModuleCallReply *r =
      RedisModule_Call(ctx, "example.hgetset", "ccc", "foo", "bar", "baz");
  RMUtil_Assert(RedisModule_CallReplyType(r) != REDISMODULE_REPLY_ERROR);

  r = RedisModule_Call(ctx, "example.hgetset", "ccc", "foo", "bar", "bag");
  RMUtil_Assert(RedisModule_CallReplyType(r) == REDISMODULE_REPLY_STRING);
  RMUtil_AssertReplyEquals(r, "baz");
  r = RedisModule_Call(ctx, "example.hgetset", "ccc", "foo", "bar", "bang");
  RMUtil_AssertReplyEquals(r, "bag");
  return 0;
}

// Unit test entry point for the module
int TestModule(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  RMUtil_Test(testParse);
  RMUtil_Test(testHgetSet);

  RedisModule_ReplyWithSimpleString(ctx, "PASS");
  return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {

  // Register the module itself
  if (RedisModule_Init(ctx, "dehydrator", 1, REDISMODULE_APIVER_1) ==
      REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  // registerdehydrator.push - using the shortened utility registration macro
  RMUtil_RegisterWriteCmd(ctx, "dehydrator.push", PushCommand);

  // registerdehydrator.pull - using the shortened utility registration macro
  // RMUtil_RegisterWriteCmd(ctx, "dehydrator.pull", PullCommand);

  // registerdehydrator.poll - using the shortened utility registration macro
  RMUtil_RegisterWriteCmd(ctx, "dehydrator.poll", PollCommand);


  // register the unit test
  RMUtil_RegisterWriteCmd(ctx, "dehydrator.test", TestModule);

  return REDISMODULE_OK;
}
