This project is a lightweight multi-threaded server framework. You can quickly build a server program by simply registering your business handling interfaces.
The framework reserves a USDT probing interface for later functional and performance probing, and support for RPC is being added in parallel.

Framework features:
1. Multi-threaded lock-free architecture — each worker thread has its own independent thread context, so there is no data contention.
2. Support for user-defined main function bodies for worker threads.
3. Support for user-defined command-parsing functions for the RPC server.
4. Support for SSL-encrypted connections.
5. Support for address proxying.
6. Pre-embedded USDT static probe interfaces for convenient sampling and observation.
