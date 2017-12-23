# redo - An implementation of djb's redo

# Installation

Like any self-respecting compiler is written in the language it compiles,
redo is used to build itself.
This is done using a simplistic POSIX Shell implementation of redo located
in the bootstrap/ directory.
You might want to edit the file config.sh to suite your needs.

    $ bootstrap/redo clean install

# Dependencies
- skalibs >= 2.5.0.0	: http://skarnet.org/software/skalibs/

# Changes to djb's original design
## The "\_" wildcard
djb intended the string "default" to be used as a wildcard string, e.g. "default.o.do" when looking for dofiles.
This implemenation uses both "default" and "\_" (preferring the latter), e.g. "\_.o.do"
## Dependencies
djb mentions three different kinds of dependencies: source-, target- and absent-files.
This implemenation additionally uses "virtual" in it's prereqs to indicate targets that do not produce output.

# credits
## djb
for his original [design notes](https://cr.yp.to/redo.html)

# similar work
## Avery Pennarun's implementation of redo in Python
https://github.com/apenwarr/redo

## Chris Forno's implemenation of redo in Haskell
https://github.com/jekor/redo

## Jonathan de Boyne Pollard's implementation of redo in C++
https://jdebp.eu/Softwares/redo/
