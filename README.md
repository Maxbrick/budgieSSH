# budgieSSH

It's an SSH client for the 3DS that actually works

Sorry S3SH but you don't work no more :(

also it has a cute bird named Jimmy

To use this on your 3DS, you need the homebrew launcher. If you don't have the homebrew launcher,
follow the instructions at https://3ds.hacks.guide/

After that, download the 3dsx file from the latest version in the Releases page and copy it to your
3DS' sd card in the "3ds" folder.

# Pubkey Authentication
since this was built in mbedtls, use RSA keys. Formats like edwhateveritcalled won't work.

I am still unsure ifkey authentication works at all... please, try it and tell me how it goes.
# Building

You need devkitpro and libctru. You also need to install 3ds-mbedtls with (dkp-)pacman.

Then, cross-compile libssh2 with 3ds-mbedtls as a crypto backend.
to do that, source 3dsvars.sh, then run ./configure, disabing binaries and specifying the respective prefixes.
