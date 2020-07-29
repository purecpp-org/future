Future文档

# 背景和动机

## 标准库中future存在的问题
C++11标准中提供了std::future和std::promise，但是标准库的future无法实现链式调用，无法满足异步并行编程的场景。

## callback hell
在异步回调中继续调用异步接口，当异步调用嵌套层次很深的时候就形成了callback hell
![callback hell](callbackhell.png)

我们需要一种更好的future来解决前面提到的两个问题，新的future将提供完备的异步并行场景的接口，这也是新future开发的动机。

# 基本概念

## future
表示一个异步返回的对象，还没有被计算的结果，一个未来的值。

## promise
一个异步返回对象(future)的创建者，也是移步结果的创建者，当promise设置值后会触发future取值的返回。

## 共享状态

promise和future共享一个内部状态(shared state)，future等待这个shared state的值，promise设置shared state的值。

![shared state](sharedstate.png)

## promise和future的关系
promise和future通过一个内部的共享状态关联，future等待共享状态的值，promise设置共享状态的值，可以把promise看作一个生产者，future看作一个消费者。

![promise-future](promise.png)


# 快速示例

# 如何创建future

ready future

promise future

async

# 获取future值

# future then

then

lauch policy

executor/executor adaptor

# 异常处理

# WhenAll/WhenAny

# 总结