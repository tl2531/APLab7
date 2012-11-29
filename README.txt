Emily Schultz
ess2183
Lab 7

Part 1: Webpage
This part of my assignment works as specified. The url for my webpage is:
http://www.cs.columbia.edu/~ess2183/cs3157/tng/
My webpage is identical to Jae's.

Part 2: Web server
Part a: Serving static contents
This part of my program works as directed. I ran valgrind on my program, testing using netcat and Safari, and the end result was All heap blocks were freed - no leaks are possible (0 errors from 0 contexts)

Part b: Serving Dynamic Contents
This part of my program doesn't quite work as directed. I ran valgrind and there were no errors (but some still reachable, which was okay). The only problem is if you try to access a second mdb-lookup key value after you have done one already it will hang. The rest of this part works as directed, and it is only a minor error. You can still access other parts, like the tng page.
I know the error involves how I detect the final '\n', but after trying a lot of ways I couldn't get it to work.
