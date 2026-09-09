import time

with open( "include/buildInfo.h", "w" ) as f:
    f.write( f'#define BUILD_TIMESTAMP "{time.strftime("%Y-%m-%d %H:%M:%S")}"\n' )