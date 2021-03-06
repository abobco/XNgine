# XNgine

Lua host for game development on Raspberry Pi single-board computers. Developed to prioritize performance on the Raspberry Pi Zero W, but also runs with no issues on all models of Raspberry Pi 1, 2, & 3.

![screenshot](/screen_caps/demo.png)

## usage:

Clone this onto a raspi & try an example script
```
git clone https://github.com/abobco/XNgine.git

cd XNgine/build && chmod 555 XNgine
sudo ./XNgine ../lua/tests/3d.lua
```
* there is 1 command line argument: a relative path to the game script to run

* if no script argument is provided, it will default to `../lua/main.lua`

* 2 special functions you can define anywhere in your script to be called automatically by XNgine:
    * `_draw()` : called once per frame update
    * `_fixedUpdate()` : called at 60 Hz (frame rate independent)

* if you have a keyboard connected while XNgine is running, press the **`** key to open an interactive lua console (load new scripts, access your variables/functions) ((only tested over SSH from windows)) 

* scripts run on [Lua 5.4](http://www.lua.org/manual/5.4/manual.html) with a custom patch to support compound assignment operators (+=, *=, etc)

* example scripts and documentation in [lua/tests/](/lua/tests)

## notes about websockets

XNgine includes an optional webserver, which hosts a simple website that functions as a motion controller for mobile devices (iOS, Android). Gyroscope events and virtual button presses are sent to the server via websocket connections. 

Since HTTPS is required to collect motion data, you can use **certbot** to generate free TLS certificates for the server. To install this on your raspi:
```
sudo apt-get install certbot -y
```
To generate a certificate:
```
sudo certbot certonly --standalone
```
You will be prompted for a domain name, make sure you provide one that meets these requirements:

* DNS points to the **public IP address for your router**
* Your router **forwards port 80 to the raspi's local IP address**

If done correctly, you should see something like this:
```
Congratulations! Your certificate and chain have been saved at:
/etc/letsencrypt/live/www.studiostudios.net/fullchain.pem
Your key file has been saved at:
/etc/letsencrypt/live/www.studiostudios.net/privkey.pem
```
**NOTE**: You also need to edit the **`WEBSOCKET_DOMAIN`** variable in `lua/settings.lua` to match the domain that you certified

![server-demo](screen_caps/server-demo.gif)

[1080p video of above gif](https://drive.google.com/file/d/1W-RWoS0H4cAkrVpk8nQc-kJrwsY4upS6/view?usp=sharing)

## dependencies
This project uses the following open source C libraries (prebuilt, located in `build/_deps`):

* [raylib](https://github.com/raysan5/raylib) - simple game development library 
* [libwebsockets](https://github.com/warmcat/libwebsockets) - minimal websocket library
* [lua 5.4](http://www.lua.org/source/5.4/index.html) with [this compound assignment power patch](http://lua-users.org/files/wiki_insecure/power_patches/5.4/plusequals-5.4.patch) (support for +=, -=, etc)