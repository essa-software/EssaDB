Feel invited to contribute to Essa Software products. You can do it by pull requests as well as u can contact us and be a part of Essa Software. 
If you can programm in C or C++, want to do something interesting and work together, there's a place for you. There's plenty of work there and
I'm certain that this isn't our last common project. Looking forward to future contributing with you guys!

These contribution guidelines and coding styles are heavily inspired by [SerenityOS contributing guidelines](https://github.com/SerenityOS/serenity/blob/master/CONTRIBUTING.md).

## Some basic rules

If you decide to contribute, read our [coding style](./docs/CodingStyle.md).

### Describe your commits, what they do and why.

* Every commit should contain a category (directory name e.g `Core`, or `Build`, or `README` or something other meaningful)
* Commit title and description should be wrapped at 72 characters.
* Don't use 

Bad example:

```
fix bug
```

Good example:

```
Core: Don't prematurely dereference column in ORDER BY

Fixes segfault when accessing nonexistent column in GROUP BY.

Also add some tests for ORDER BY which cover this case.
```

### Squash your commits and avoid conflicts

- Every commit should change as little as possible, but compile - this simplifies bisecting.
- You can use `git rebase` for that (see [this video](https://youtu.be/ElRzTuYln0M) for a tutorial on rewriting git history)
- Avoid merge commits because they look awful in commit log.
- Make sure your commits are rebased on the master branch to avoid merge conflicts

### Write in C++20, not in C

Avoid things like manual memory management, plain C arrays, and deprecated features (this one is obvious ig)
