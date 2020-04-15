# WebSocket MPV Webui
...is a web based user interface with controls for the [mpv mediaplayer](https://mpv.io/).

This project is a fork of [simple-mpv-webui](https://github.com/open-dynaMIX/simple-mpv-webui).  
The webserver is re-implemented using [libwebsockets](https://libwebsockets.org/) C library.  

## Usage
To use it, you'll need to compile the plugin:  
`$ make`

Then, copy the library (`ws-webui.so`) and the directory `ws-webui-page` to `~/.config/mpv/scripts`.  
Alternatively you can also use the `--script` option from mpv or add something like  
`scripts-add=/path/to/simple-mpv-webui/ws-webui.so` to `mpv.conf`.
  
You can access the webui when accessing [http://127.0.0.1:8080](http://127.0.0.1:8080) or  
[http://[::1]:8080](http://[::1]:8080) in your webbrowser.  
By default it listens on `0.0.0.0:8080` and `[::0]:8080`. As described below, this can be changed.  

### Options
Options can be set with [--script-opts](https://mpv.io/manual/master/#options-script-opts)  
with the prefix `ws-webui-`.

#### port (int)
Set the port to serve the webui (default: 8080).

Example:

```
ws-webui-port=8000
```

#### ipv6 (bool)
Enable/disable listening on ipv6 (default: yes)

Example:

```
ws-webui-ipv6=no
```

#### interface (string)
The interface to which the http and websocket severs are bound (default: all).  
You can limit binding to a specific interface (see `ip link show`).

Example:

```
ws-webui-interface=eth0
```

#### dir (string)
The name of the directory containing web files (static http hosting).  
Not intended as absolute path. It's relative to `ws-webui.so` file.  
Default is `ws-webui-page`, but you can change whenever you need to.

Example:

```
ws-webui-dir=ws-webui-page-mod
```

### Authentication
Support not planned.

## Dependencies
The code was tested on GNU/Linux only.  
Specifically on [Arch Linux](https://www.archlinux.org/).

### Linux

 - [libwebsockets](https://libwebsockets.org/) (`pacman -S libwebsockets`)

## Thanks
Thanks to [open-dynaMIX](https://github.com/open-dynaMIX) for his work on this.

## Differences to simple-mpv-web-ui
 - More media controls (namely: sub-seek, sub-scale)
 - mpv controls are exposed through a websocket backend instead of HTTP+REST backend (static files are still served with a simple HTTP server)
 - C backend

## Licensing
The C code of this project is released under GPLv3 license [[1]](LICENSE).  
The README and webui files keep the same license of the upstream project and were modified just to make the UI works with websockets [[2]](ws-webui-page/LICENSE).
 
## Warning
This project was done for educational purposes and for fun.
