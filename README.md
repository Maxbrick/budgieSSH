# budgieSSH

It's an SSH client for the 3DS that actually works

Sorry S3SH but you don't work no more :(

also it has a cute bird named Jimmy

To use this on your 3DS, you need the homebrew launcher. If you don't have the homebrew launcher,
follow the instructions at https://3ds.hacks.guide/

After that, download the 3dsx file from the latest version in the Releases page and copy it to your
3DS' sd card in the "3ds" folder.

# Pubkey Authentication
Just a heads up, while I haven't figured it out just yet, I believe that pubkey authentication can indeed work.
The issue is that most people use OpenSSH keys using ssh-keygen or other programs.

Since this client is made with the 3ds-mbedtls as a crypto backend, my speculation is that you probably just need
to figure out how to make a key pair with mbedtls.

The program searches sdmc:/3ds/ssh for key files.
