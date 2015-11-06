# rrlite

[![Build Status](https://travis-ci.org/ropensci/rrlite.png?branch=master)](https://travis-ci.org/ropensci/rrlite)
[![Coverage Status](https://coveralls.io/repos/ropensci/rrlite/badge.svg?branch=master)](https://coveralls.io/r/ropensci/rrlite?branch=master)

R interface to [rlite](https://github.com/seppo0010/rlite).  rlite is a "self-contained, serverless, zero-configuration, transactional redis-compatible database engine. rlite is to Redis what SQLite is to SQL.

This package is designed to be used as a driver for [`RedisAPI`](https://github.com/ropensci/RedisAPI), providing only the glue code to make this work.

# Usage

See [`RedisAPI`](https://github.com/ropensci/RedisAPI) and  [`redux`](https://github.com/richfitz/redux) for more details.  The main function here is `rrlite::hirlite` that creates a `redis_api` object that exposes the full Redis API, or for use with `RedisAPI::rdb`.

## Redis level

Provides a reasonably complete interface to raw redis commands:


```r
db <- rrlite::hirlite(":memory:")
db$SET("foo", "bar")
```

```
## [Redis: OK]
```

```r
db$GET("foo")
```

```
## [1] "bar"
```

```r
db$KEYS("*")
```

```
## [[1]]
## [1] "foo"
```

# Approach

This package aims to be a drop-in self-contained replacement for `redux` without requiring  `Redis` server.  Therefore almost the entire package (and tests) is automaticaly generated from `redux`.  The only installed files not generated are:

* R/hirlite.R (because documentation)
* src/subscribe.c (just a stub)

## Meta

* Please [report any issues or bugs](https://github.com/ropensci/rrlite/issues).
* License: GPL
* Get citation information for `rrlite` in R by doing `citation(package = 'rrlite')`

[![rofooter](http://ropensci.org/public_images/github_footer.png)](http://ropensci.org)
