# Vin

A vi like text editor, also French for wine.

## Why?

There's really no good reason to use this editor. Vim is a far superior vi in
every way. This editor was created as a joke. When working with docker
containers, I always found myself needing to `apt install vim` over and over
again because my DevOps friend and colleague was adamant that text editors
should not be installed in containers.

According to him:

> The whole point is to make the container small right? And vim is way too big
> that it takes up too much space in a container.

I'll show you! I thought. I'll make the smallest useful text editor I can!

And that, my dear readers, is how I spent a solid two days one Spring, while
listening to an Indiana Jones marathon in the background.

In the end, after leveraging compiler options to reduce the size and `strip`ing
the binary of anything that isn't executable code, I was left with a text editor
that was a mere 18KB in size, which would comfortably fit in a Commodore 64 from
1982\. There may be arguments for not including text editors in docker containers,
but given that one can make a useable text editor fit in 18K of RAM, size is not
one of them!

```
$ ls -lh vin
-rwxrwxr-x 1 kyle kyle 18K May  4 21:31 vin
```
