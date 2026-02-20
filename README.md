# µmacs (micromacs)

I want to build an editor that is the exact same Emacs setup I use day to day, but without the baggage of the past to allow faster development and higher quality. Elisp is nice but Scheme is better. And I want to see how small and simple this can be.

## Naive Roadmap

This is the high level plan—it doesn't contain all the difficulties and pain I'll face. I might skip some steps, like doing 4 after 2, and skip 3.

1. Delete everything from Emacs except the C core.
2. Gradually add back things needed by the C core to compile.
3. Continue bringing back files until we have a minimally usable text editor, as little code as possible.
4. Replace elisp with µlisp, a Scheme dialect for µmacs.
5. Clean up the C core.
6. Rewrite everything again.
7. Put the new codebase in a new fresh repository, and release under a BSD license instead of GPL.

## Moving forward without looking back

µmacs won't support any other OS/architecture other than macOS on Apple Silicon. Also it won't have GUI support; it's going to be a terminal-only editor, and if required, it will be compatible only with Kitty. At least at first.

I'll implement support for Linux if I start using PCs more. A GUI would require especial care, so I'll only work on that if it turns out to be worth it, it would be nice to have visuals for things like technical notebooks using µlisp. Or for typesetting documents, which is actually high priority for me. We will see how it goes.

---

Original Emacs codebase ended here: https://github.com/Jaiheravi/umacs-zero/tree/9176826f41d562f42159b402d8b1d3a676ed99a3
