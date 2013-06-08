Hacking and Contributions
=========================

We use the same coding style and guidelines as libopencm3. Please see the
HACKING in the libopencm3 directory after you have initialized your git
submodules.


Commit messages
---------------

For all aspects of Git to work the best, it's important to follow these simple
guidelines for commit messages:

1. The first line of the commit message has a short (less than 65 characters,
    absolute maximum is 75) summary
2. The second line is empty (no whitespace at all)
3. The third and any number of following lines contain a longer description of
    the commit as is neccessary, including relevant background information and
    quite possibly rationale for why the issue was solved in this particular
    way. These lines should never be longer than 75 characters.
4. The next line is empty (no whitespace at all)
5. A Signed-off-by: line according to the development guidelines

Please do not create Signed-off-by: manually because it is boring and
error-prone. Instead, please remember to always use git "commit -s" to have git
add your Signed-off-by: automatically.

* These guidelines have been adapted from http://www.coreboot.org/Git

Sign-off Procedure
------------------

We employ a similar sign-off procedure for vultureprog as the Linux developers
do. Please add a note such as

> Signed-off-by: Random J Developer <random@developer.example.org>

to your email/patch if you agree with the following Developer's Certificate of
Origin 1.1. Patches without a Signed-off-by cannot be pushed to the official
repository. You have to use your real name in the Signed-off-by line and in any
copyright notices you add. Patches without an associated real name cannot be
committed!

<pre><code>
 Developer's Certificate of Origin 1.1:
 By making a contribution to this project, I certify that:

 (a) The contribution was created in whole or in part by me and I have
 the right to submit it under the open source license indicated in the file; or

 (b) The contribution is based upon previous work that, to the best of my
 knowledge, is covered under an appropriate open source license and I have the
 right under that license to submit that work with modifications, whether created
 in whole or in part by me, under the same open source license (unless I am
 permitted to submit under a different license), as indicated in the file; or

 (c) The contribution was provided directly to me by some other person who
 certified (a), (b) or (c) and I have not modified it; and

 (d) In the case of each of (a), (b), or (c), I understand and agree that
 this project and the contribution are public and that a record of the contribution
 (including all personal information I submit with it, including my sign-off) is
 maintained indefinitely and may be redistributed consistent with this project or the
 open source license indicated in the file.
</code></pre>

Note: The Developer's Certificate of Origin 1.1 is licensed under the terms of
the Creative Commons Attribution-ShareAlike 2.5 License.

* Guidelines adapted from http://www.coreboot.org/Development_Guidelines
