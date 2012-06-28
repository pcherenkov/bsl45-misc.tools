/* MacOS X: g++-mp-4.6 -I/usr/local/include/luajit-2.0 -L/usr/local/lib -lobjc -lluajit-51 -pagezero_size 10000 -image_base 100000000 -fobjc-exceptions -o ljc.tt ./luacrash.m */
/* Ubuntu Linux: g++ -lobjc -lluajit-5.1 -I/usr/include/luajit-2.0 -fobjc-exceptions -Wl,--no-as-needed -o ljc.tt ./luacrash.m */

#include <objc/Object.h>
#include <objc/runtime.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


@interface Exception: Object {
    char *errmsg;
}
+ (id) alloc;
- (id) init: (char*) msg;
@end

@implementation Exception
+ (id) alloc
{
    static char buf[sizeof(Exception)];
    Exception *new = (Exception *) buf;
    object_setClass(new, self);
    return new;
}

- (id) init: (char*) msg;
{
    self = [super init];
    errmsg = msg;
    return self;
}
@end

void test(struct lua_State *L)
{
    const char *format = luaL_checkstring(L, 1);
}

static int
box_lua_panic(struct lua_State *L)
{
    @throw [[Exception alloc] init: "message"];
}

void main()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_atpanic(L, box_lua_panic);
    @try {
        test(L);
    } @catch (Exception *e) {
        printf("exception handled\n");
    } @catch (...) {
        printf("catch all is king!\n");
    }
    lua_close(L);
}

/* __EOF__ */

