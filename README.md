# iPlug 2 - SK support
### C++ audio plug-in framework for desktop, mobile (iOS) and web with support for SuperKraft

[Read the original iPlug2 README here](https://github.com/iPlug2/iPlug2/)

Added support for SuperKraft in iPlug2 apps and VST's.

What's different:

- Proper Two-Way IPC asynchronous communication

- (WIP) Proper content load routing
    - Allows you to reload the page to load your latest frontend changes
    - Routing to load from disk when running in `DEBUG` and load from bundle when running in `RUNTIME`
    
- (WIP) A virtual JS backend for support of NodeJS- and ElectronJS-like coding.
    - Wrappers for NodeJS modules such as `fs` (sync & async), `ElectronJS`, `process`, more...

### What is the "virtual JS backend" (VJSB)?

The virtual JS backend is a layer where you code in JS, just like you would in NodeJS, that communicates with the C++ backend and with other WebViews in your app.
This means that the same code you write for your NodeJS app would be compatible with your iPlug2 app.

### What's the benefit of using the VJSB and how should I use it?

The recommended use of the VJSB is to handle your non-critical logic in JS, while handling your critical logic in C++.

Additionally, you get access to the vast amount of NodeJS modules using NPM that you can use in the VJSB.

The VJSB can be considered as a NodeJS substitute, but without the original NodeJS code.
