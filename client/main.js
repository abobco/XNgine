let  player = 0;

// this is sum bullshit right here
let MSG_DISCONNECT = -1,
    MSG_CONNECT = 0,
    MSG_MOTION_VECTOR = 1,
    MSG_STRING = 2,
    MSG_QUIT = 3,
    MSG_BTN_A = 4,
    MSG_BTN_B = 5,
    MSG_BTN_A_UP = 6,
    MSG_BTN_B_UP = 7;

// subtract this from the rotation sent to the server
// set when the user presses the calibrate-button
let current_pos = {x:0, y:0, z:0};
let offset = {x:180, y:0, z:0};
let prev_rot = {beta:0, gamma:0, alpha:0};

let derived_rot = {x:1, y:0, z:0, w:0};

let sensor;

function get_appropriate_ws_url(extra_url)
{
    var pcol;
    var u = document.URL;

    /*
    * We open the websocket encrypted if this page came on an
    * https:// url itself, otherwise unencrypted
    */

    if (u.substring(0, 5) === "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) === "http")
            u = u.substr(7);
    }

    u = u.split("/");

    /* + "/xxx" bit is for IE10 workaround */
    return pcol + u[0] + "/" + extra_url;
}

function new_ws(urlpath, protocol) {
    let ws = new WebSocket(urlpath, protocol);
    ws.binaryType = "arraybuffer"
    return ws;
}  

function send_btn_event(ws, player, btn_type) {
    // make a byte buffer for the data
    let buffer = new ArrayBuffer(8);
    let typed_array = new Int32Array(buffer);
    
    // quit message values
    typed_array[0] = btn_type;
    typed_array[1] = player;

    // write to the socket
    ws.send(buffer);  
}

function gyro_to_quaternion( event, prev_rotation ) {
    let EPSILON = 1E-4;

    let axisX = event.beta - prev_rotation.beta;
    let axisY = event.gamma - prev_rotation.gamma;
    let axisZ = event.alpha - prev_rotation.alpha;

    // axisX *= Math.PI/180;
    // axisY *= Math.PI/180;
    // axisZ *= Math.PI/180;

    deltaRotationVector = [];

    let omegaMagnitude = Math.sqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);
    console.log("OM: ", omegaMagnitude);

    // Normalize the rotation vector if it's big enough to get the axis
    if ( omegaMagnitude > EPSILON ) {
        axisX /= omegaMagnitude;
        axisY /= omegaMagnitude;
        axisZ /= omegaMagnitude;
    }

    // Integrate around this axis with the angular speed by the timestep
    // in order to get a delta rotation from this sample over the timestep
    // We will convert this axis-angle representation of the delta rotation
    // into a quaternion before turning it into the rotation matrix.
    const thetaOverTwo = omegaMagnitude / 2.0;
    console.log("interval", event.interval);
    const sinThetaOverTwo = Math.sin(thetaOverTwo);
    const cosThetaOverTwo = Math.cos(thetaOverTwo);
    deltaRotationVector.push(sinThetaOverTwo * axisX);
    deltaRotationVector.push(sinThetaOverTwo * axisY);
    deltaRotationVector.push(sinThetaOverTwo * axisZ);
    deltaRotationVector.push(cosThetaOverTwo);

    return deltaRotationVector;

    derived_rot.x += sinThetaOverTwo * axisX;
    derived_rot.y += sinThetaOverTwo * axisY;
    derived_rot.z += sinThetaOverTwo * axisZ;
    derived_rot.w += cosThetaOverTwo;

}

function motion_setup(ws) {
     // unhide buttons
     document.getElementById('buttons').classList.remove('hidden');
     document.getElementById('permission-button').classList.add('hidden');

     // gyroscope events
     window.addEventListener('deviceorientation', (event) => {
        // cube rotation
        document.getElementById('cube').style.webkitTransform =
        document.getElementById('cube').style.transform =
        'rotateX(' + event.beta + 'deg) ' +
        'rotateY(' + event.gamma + 'deg) ' +
        'rotateZ(' + event.alpha + 'deg)';

        // angle values (in degrees)
        document.getElementById('beta').innerHTML = Math.round(event.beta);
        document.getElementById('gamma').innerHTML = Math.round(event.gamma);
        document.getElementById('alpha').innerHTML = Math.round(event.alpha); 
         
        // derived_rot.beta  = event.rotationRate.beta;
        // derived_rot.gamma = event.rotationRate.gamma;
        // derived_rot.alpha = event.rotationRate.alpha;

        // alternate method, get a quaternion from integration
        deltaRot = gyro_to_quaternion(event, prev_rot);
        derived_rot.x += deltaRot[0];
        derived_rot.y += deltaRot[1];
        derived_rot.z += deltaRot[2];
        derived_rot.w += deltaRot[3];

        document.getElementById('acceleration-including-gravity-x').innerHTML =
        (derived_rot.x).toFixed(2);
        document.getElementById('acceleration-including-gravity-y').innerHTML =
        (derived_rot.y).toFixed(2);
        document.getElementById('acceleration-including-gravity-z').innerHTML =
        (derived_rot.z).toFixed(2);
        
        // convert to -180 to 180 range
        let x = (( event.alpha + event.gamma*(event.beta/90) ) % 360) - 180 ;
        // if ( x > 180 ) x -= 360;
        // let y = event.beta + event.gamma*(event.gamma/90);
        // if ( Math.abs(event.beta - prev_rot.beta) > 20 )
        //     offset.y += (event.beta - prev_rot.beta);
        let y = event.beta;
        let z = event.gamma;
        // if ( y > 180) y -= 360;
        // if (y < -180) y += 360;
        current_pos.x = x;
        current_pos.y = y;
        current_pos.z = z;

        // subtract calibration offset
        x -= offset.x;
        y -= offset.y; 
        z -= offset.z;
        
        if ( x > 180 )
            x -= 360;
        else if ( x < -180 )
            x +=360;
        // both axes are inverted
        x*=-1;
        y*=-1;

        // show derived game position
        document.getElementById('x').innerHTML = Math.round(x);
        document.getElementById('y').innerHTML = Math.round(y); 

        // make a byte buffer to send the data
        let buffer = new ArrayBuffer(20);
        let typed_array = new Int32Array(buffer);
        
        // convert floats to ints
        const conv_factor = Math.PI/360.0*1E7;
        typed_array[0] = MSG_MOTION_VECTOR;
        typed_array[1] = player;
        typed_array[2] = Math.round(x*conv_factor);
        typed_array[3] = Math.round(y*conv_factor);
        typed_array[4] = Math.round(z*conv_factor);
        
        // typed_array[2] = Math.round(-(derived_rot.z - offset.x)*conv_factor*2);
        // typed_array[3] = Math.round(-(derived_rot.x - offset.y)*conv_factor*2);

        // write to the socket
        ws.send(buffer);  

        prev_rot.beta = event.beta;
        prev_rot.gamma = event.gamma;
        prev_rot.alpha = event.alpha;
     });

     // accelerometer events
     window.addEventListener('devicemotion', (event) => {    
         // linear acceleration  
         document.getElementById('acceleration-x').innerHTML =event.acceleration.x.toFixed(2);
         document.getElementById('acceleration-y').innerHTML = event.acceleration.y.toFixed(2);
         document.getElementById('acceleration-z').innerHTML = event.acceleration.z.toFixed(2);

        //  // raw acceleration
        //  document.getElementById('acceleration-including-gravity-x').innerHTML =
        //  event.accelerationIncludingGravity.x.toFixed(2);
        //  document.getElementById('acceleration-including-gravity-y').innerHTML =
        //  event.accelerationIncludingGravity.y.toFixed(2);
        //  document.getElementById('acceleration-including-gravity-z').innerHTML =
        //  event.accelerationIncludingGravity.z.toFixed(2);

        // rotation rate
        document.getElementById('rotation-rate-beta').innerHTML = Math.round(event.rotationRate.beta);
        document.getElementById('rotation-rate-gamma').innerHTML = Math.round(event.rotationRate.gamma);
        document.getElementById('rotation-rate-alpha').innerHTML = Math.round(event.rotationRate.alpha);

        // derived_rot.beta  += event.rotationRate.beta;
        // derived_rot.gamma += event.rotationRate.gamma;
        // derived_rot.alpha += event.rotationRate.alpha;

        //  // update interval
        //  document.getElementById('interval').innerHTML = event.interval.toFixed(5);
     });
}
let debug = false;
document.addEventListener("DOMContentLoaded", () => {    
    var ws = new_ws(get_appropriate_ws_url(""), "lws-minimal");
    console.log(ws.binaryType);
    try {
        ws.onopen = ( evt ) => { };

        ws.onmessage = function got_packet(msg) {           
            if ( !player ) { // 1st message we received

                let data_view = new Int32Array(msg.data);
                player = data_view[0]; // set player #
                console.log(player);
                
                const pb = document.getElementById('permission-button');
                // add a callback function
                pb.addEventListener('click', () => {
                    // feature detect
                    if (typeof DeviceMotionEvent.requestPermission === 'function') {
                        // This opens a popup asking for permission & returns a promise
                        DeviceMotionEvent.requestPermission().then( permissionState => { // attach a callback to the promise    
                            if (permissionState === 'granted') 
                                motion_setup(ws);   // start sending gyro data
                        }).catch(console.error);
                    } else {
                        motion_setup(ws);
                    }
                });
                
            }
        };

        ws.onclose = function() {};
        
        // ---------------- button callbacks -----------------------------------

        const permission_btn = document.getElementById('permission-button');
        permission_btn.addEventListener('click', () => {
            // feature detect
            if (typeof DeviceMotionEvent.requestPermission === 'function') {
                // This opens a popup asking for permission & returns a promise
                DeviceMotionEvent.requestPermission().then( permissionState => { // attach a callback to the promise    
                    if (permissionState === 'granted')  {
                        
                    };
                        motion_setup(ws);   // start sending gyro data
                }).catch(console.error);
            } else {
                motion_setup(ws);
            }
        });
        
        const quit_btn = document.getElementById('quit-button');
        quit_btn.addEventListener('click', () => {
            // make a byte buffer for the data
            let buffer = new ArrayBuffer(8);
            let typed_array = new Int32Array(buffer);
            
            // quit message values
            typed_array[0] = MSG_QUIT;
            typed_array[1] = player;

            // write to the socket
            ws.send(buffer);  
        });

        const debug_btn = document.getElementById('debug-button');
        debug_btn.addEventListener('click', () => {
            debug=!debug;

            if ( debug ) {
                document.getElementById('do-info').classList.remove('hidden');
                document.getElementById('dm-info').classList.remove('hidden');
            }
            else {
                document.getElementById('do-info').classList.add('hidden');
                document.getElementById('dm-info').classList.add('hidden');
            }
        });

        const calibrate_btn = document.getElementById('calibrate-button');
        calibrate_btn.addEventListener('click', () => {
            offset.x = current_pos.x;
            offset.y = current_pos.y;
            offset.z = current_pos.z;

            // offset.x = derived_rot.z;
            // offset.y = derived_rot.x;
        });

        const a_btn = document.getElementById('a-button');
        a_btn.addEventListener('touchstart', () => {
           send_btn_event(ws, player, MSG_BTN_A);
        });
        a_btn.addEventListener('touchleave', () => {
            send_btn_event(ws, player, MSG_BTN_A_UP);
        });
        a_btn.addEventListener('touchend', () => {
            send_btn_event(ws, player, MSG_BTN_A_UP);
        });

        const b_btn = document.getElementById('b-button');
        b_btn.addEventListener('mousedown', () => {
            // make a byte buffer for the data
            let buffer = new ArrayBuffer(8);
            let typed_array = new Int32Array(buffer);
            
            // quit message values
            typed_array[0] = MSG_BTN_B;
            typed_array[1] = player;

            // write to the socket
            ws.send(buffer);             
        });

        // ---------------------------------------------------------------------
    } catch(exception) {
        alert("<p>Error " + exception);  
    }
    
}, false);
