For the past month or so, I've been writing a PNG parser as a learning exercise, but it also ended up being a perfect tool for me to test writing code with exceptions like how I thought I'd write code. Somewhere along the way I also saw this as a perfect opportunity to provide benchmark results of practical usage of exceptions compared to `std::expected` though I used `tl::expected` as I'm currently not using C++23.

The benchmarks that I can easily find online tend to show very trivial code to benchmark exceptions. Here are the first 2 posts I got from just searching "C++ exception benchmark" for example.

* [The real cost of exception handling in C++ with benchmark - DEV Community](https://dev.to/visheshpatel/the-real-cost-of-exception-handling-in-c-with-benchmark-kd)
* [Investigating the Performance Overhead of C++ Exceptions | PSPDFKit](https://pspdfkit.com/blog/2020/performance-overhead-of-exceptions-in-cpp/)

By no means would I think this is considered real world use of exceptions and instead more something akin of trying to benchmark the overhead of a virtual function call compared to a non-virtual one where the bodies of the function are empty. So with that, I provide to you my benchmark results.

# Disclaimer

By no means is my testing methodology perfect, but hopefully they are better than the examples I've provided.

Code can be found here  
- [Exception Branch](https://github.com/XeroKimo/PNGParser/tree/benchmark_exception_branch)
- [Expected w/ std::shared_ptr](https://github.com/XeroKimo/PNGParser/tree/benchmark_expected_ptr_branch)
- [Expected w/ enum](https://github.com/XeroKimo/PNGParser/tree/benchmark_expected_branch)

# Methodolgy

This was tested on MSVC x64 Release

There are 3 different branches that is provided:

* `Exceptions`
* `Expected` w/ Error type being `std::shared_ptr<std::exception>>`
* `Expected` w/ Error type being a `enum`

Why do I have 2 expected branches? The one using `std::shared_ptr` exists because I'd like to emulate exceptions as much as possible, so it's purpose is to try to match the drawbacks of exceptions, such as being heap allocated, and being ref counted. The `enum` branch is there to play with `Expected`'s strength. So hopefully this paints a better picture of the overhead of each approach.

For each branch, I benchmarked the following 100 times on the same image, and marking the Min, Max, Avg time of parsing:

* No failures
* Unimportant chunks failed
* All chunks failed
* PNG signature failed
* Abort on header chunk failure
* Abort on first unimportant chunk failure
* Abort on header chunk failure (No Seek)
* Abort on first unimportant chunk failure (No Seek)
* Deepest Callstack Test

Currently in my library, if I fail to parse a chunk, I throw an exception. When I fail to parse a chunk, the stream seeks to the start of the next chunk so we can try to parse the next chunk. Which explains what the various tests are, why I chose them where just whatever I felt like testing.

NEW: Deepest Callstack Test how expensive would is it to propagate an error that has the deepest callstack. For my test case, the deepest stack is 10.

The first 2 tests will properly decode a PNG so it'll actually go through de-filtering, de-interlacing, and such as they'll have all the data they need in order to do so. They are the success cases. The rest will fail before the decoding actually starts.

# Results

Exceptions:

&#x200B;

||Min|Max|Average|
|:-|:-|:-|:-|
|No Failure|72ms|84ms|73ms|
|Unimportant Chunks Failed|73ms|94ms|75ms|
|All Chunks Failed|11ms|14ms|11ms|
|PNG Signature Failed|268us|470us|310us|
|Abort on Header Chunk Failure|275us|664us|324us|
|Abort on Unimportant Chunk Failure|284us|514us|324us|
|Abort on Header Chunk Failure (No Seek)|274us|507us|315us|
|Abort on Unimportant Chunk Failed (No Seek)|278us|553us|314us|
|Deepest Callstack Test|271us|1ms|316us|

&#x200B;

Expected w/ std::shared\_ptr\<std::exception>

&#x200B;

||Min|Max|Average|
|:-|:-|:-|:-|
|No Failure|86ms|95ms|87ms|
|Unimportant Chunks Failed|85ms|102ms|87ms|
|All Chunks Failed|136us|187us|142us|
|PNG Signature Failed|5us|26us|6us|
|Abort on Header Chunk Failure|6us|35us|8us|
|Abort on Unimportant Chunk Failure|7us|49us|9us|
|Abort on Header Chunk Failure (No Seek)|6us|34us|7us|
|Abort on Unimportant Chunk Failed (No Seek)|6us|42us|8us|
|Deepest Callstack Test|7us|64us|8us|

&#x200B;

Expected w/ enum

&#x200B;

||Min|Max|Average|
|:-|:-|:-|:-|
|No Failure|72ms|92ms|74ms|
|Unimportant Chunks Failed|73ms|81ms|74ms|
|All Chunks Failed|121us|171us|127us|
|PNG Signature Failed|4us|20us|6us|
|Abort on Header Chunk Failure|6us|40us|7us|
|Abort on Unimportant Chunk Failure|7us|35us|8us|
|Abort on Header Chunk Failure (No Seek)|5us|29us|6us|
|Abort on Unimportant Chunk Failed (No Seek)|6us|30us|7us|
|Deepest Callstack Test|6us|33us|7us|

# Observations

* Even in the all chunks failed case for exceptions, it is still faster than decoding my specific test image. This is simulating a worse case for exceptions. I found that interesting because it means, even if you fail a lot, in this case, it's still faster than succeeding.
* Expected + shared ptr has the worse performance on success cases, I wonder why? I would expect it to actually be closer to expected + enum just like the error case.
* Expected + shared ptr slowest failure case is still faster than the fastest exception failure case.
* Expected + enum success cases are actually comparable to exceptions.

# Conclusion

So exceptions are slow? For success cases, actually no, which matches up with material I've read for table based exceptions. For failure cases, expected can be about 50x faster.

So just use expected yea? Well actually, for PNG parsing, I actually think it doesn't really matter, and there are probably other cases out there, not just PNG parsing, where the failure case performance of exceptions vs expected won't matter.

There is still the ergonomics of exceptions vs expected. I spent about 4-5 hours converting exceptions to expected, and IMO, it was painful seeing all the explicit propagation. I'd still much rather use exceptions by default, and use expected for optimization where failure cases are more common than the success case and if the error case is faster than the success case. If you have common errors, but both success and failure cases have the same performance with exceptions, there's nothing to optimize, and if an error case is fast, but rarely happens, it's still going to be running faster than if we always succeeded.
