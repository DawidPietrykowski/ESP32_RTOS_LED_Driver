let led_num = 60; 
let width = window.innerWidth-120;
let height = 50;
let strip = [[]];
let anim_num = 1;
let counter = 0;
let breathing_counter = 0
let blinking_counter = 0.0
let current_num = 0;
let chase_step = 0;
let last_time = 0;
let solid_color = {
  r : 255,
  g : 0,
  b : 12
};
let domain_name = window.location.href;

let config = {
  "mode": "rainbow",
  "anim_refresh_rate": 60,
  "refresh_rate": 60,
  "brightness": 255,
  "brightness_step": 64,
  "default_animation": 0,
  "enable_touch_control": true,
  "selected_color":"0000ff",
  "spread_value":3,
  "animation_direction":1,
  "single_color": true,
  "anim_speed":100
};

let animation_texts = ["rainbow", "static", "breathing", "blinking", "chase", "police", "sparkles", "off", "received"];

let dark_color_picker = 
    new ColorPickerControl({ container: document.querySelector('.color-picker-dark-theme'), theme: 'dark' });

dark_color_picker.on('change', (color) =>  {
    selectColor(color);
});


initConfig();

function applyConfig(){
  anim_num = animation_texts.findIndex(x => x === config["mode"]);
  for (const element of document.getElementsByClassName("menu-entry")) {
    if(element.id == anim_num)
      element.classList.add("active");
    else
      element.classList.remove("active");
  }
  anim_num = parseInt(anim_num);

  let elem;
  elem = document.getElementById("animation-refresh-value");
  elem.textContent = config["anim_refresh_rate"];
  elem = document.getElementById("refresh-rate-value");
  elem.textContent = config["refresh_rate"];
  elem = document.getElementById("brightness-value");
  elem.textContent = config["brightness"];
  elem = document.getElementById("brightness-step-value");
  elem.textContent = config["brightness_step"];
  elem = document.getElementById("default-animation-value");
  elem.textContent = config["default_animation"];
  elem = document.getElementById("touch-control-value");
  elem.checked = config["enable_touch_control"];

  elem = document.getElementById("animation-speed-value");
  elem.textContent = config["anim_speed"];
  elem = document.getElementById("animation-spread-value");
  elem.textContent = config["spread_value"];
  elem = document.getElementById("animation-direction-value");
  elem.textContent = config["animation_direction"];
  elem = document.getElementById("single-color-value");
  elem.checked = config["single_color"];

  // dark_color_picker.color.h = hexToRgb("#" + config["selected_color"].r);
  // dark_color_picker.color.s = hexToRgb("#" + config["selected_color"].g);
  // dark_color_picker.color.v = hexToRgb("#" + config["selected_color"].b);
  // dark_color_picker.emit('change', dark_color_picker.color);
  // dark_color_picker.update();
}

function hexToRgb(hex) {
  var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : null;
}

function initConfig(){
  var xmlHttp = new XMLHttpRequest();
  xmlHttp.onreadystatechange = function() { 
      if (xmlHttp.readyState == 4 && xmlHttp.status == 200){
        console.log(xmlHttp.responseText);
        obj = JSON.parse(xmlHttp.responseText);
        config["mode"] = obj["mode"];
        config["selected_color"] = obj["selected_color"];
        config["anim_refresh_rate"] = obj["anim_refresh_rate"];
        config["refresh_rate"] = obj["refresh_rate"];
        config["brightness"] = obj["brightness"];
        config["brightness_step"] = obj["brightness_step"];
        config["default_animation"] = obj["default_animation"];
        config["enable_touch_control"] = obj["enable_touch_control"];
        config["anim_speed"] = obj["anim_speed"];
        config["spread_value"] = 3;
        config["animation_direction"] = 1;
        config["single_color"] = false;
        console.log(config);
        applyConfig();
  }
}
  
  xmlHttp.open( "GET", window.location.href + "config", true );
  xmlHttp.send( null );
  return xmlHttp.responseText;
}

function HSVtoRGB(h, s, v) {
  var r, g, b, i, f, p, q, t;
  if (arguments.length === 1) {
      s = h.s, v = h.v, h = h.h;
  }
  i = Math.floor(h * 6);
  f = h * 6 - i;
  p = v * (1 - s);
  q = v * (1 - f * s);
  t = v * (1 - (1 - f) * s);
  switch (i % 6) {
      case 0: r = v, g = t, b = p; break;
      case 1: r = q, g = v, b = p; break;
      case 2: r = p, g = v, b = t; break;
      case 3: r = p, g = q, b = v; break;
      case 4: r = t, g = p, b = v; break;
      case 5: r = v, g = p, b = q; break;
  }
  return {
      r: Math.round(r * 255),
      g: Math.round(g * 255),
      b: Math.round(b * 255)
  };
}



function selectColor(color){
  
  console.log(color.toRGB());
  solid_color.r = color.toRGB()[0];
  solid_color.g = color.toRGB()[1];
  solid_color.b = color.toRGB()[2];
  config["selected_color"] = color.toHEX().substring(1, 7).toLowerCase();
  console.log(config["selected_color"]);
}

function selectAnimation(animation_number){
  anim_num = parseInt(animation_number);
  var element = document.getElementById("animation-refresh-value");
  config["anim_refresh_rate"] = parseInt(element.textContent);
  var element = document.getElementById("refresh-rate-value");
  config["refresh_rate"] = parseInt(element.textContent);
  var element = document.getElementById("brightness-value");
  config["brightness"] = parseInt(element.textContent);
  var element = document.getElementById("brightness-step-value");
  config["brightness_step"] = parseInt(element.textContent);
  var element = document.getElementById("default-animation-value");
  config["default_animation"] = parseInt(element.textContent);
  var element = document.getElementById("touch-control-value");
  config["enable_touch_control"] = element.checked;

  var element = document.getElementById("animation-speed-value");
  config["anim_speed"] = parseInt(element.textContent);
  var element = document.getElementById("animation-spread-value");
  config["spread_value"] = parseInt(element.textContent);
  var element = document.getElementById("animation-direction-value");
  config["animation_direction"] = parseInt(element.textContent);
  var element = document.getElementById("single-color-value");
  config["single_color"] = element.checked;

  var xhr = new XMLHttpRequest();
  var url = domain_name;
  xhr.open("POST", url, true);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.onreadystatechange = function () {
      if (xhr.readyState === 4 && xhr.status === 200) {
          console.log("received HTTP 200");
      }
  };

  config["mode"] = animation_texts[animation_number];
  var data = JSON.stringify(config);
  console.log("sending data");
  console.log(data);
  xhr.send(data);

  for (const element of document.getElementsByClassName("menu-entry")) {
    if(element.id == animation_number)
      element.classList.add("active");
    else
      element.classList.remove("active");

      console.log("animation_number: " + animation_number);
      console.log("anim_num: " + anim_num);
}
}

function apply_animation(){
  console.log("apply_animation anim_num: " + anim_num);
  switch(anim_num){
    case 0:
      counter += ((millis()-last_time)/1000)*60;
      last_time = millis();
      for(let i = 0; i < led_num; i++){
        let obj = HSVtoRGB(i/led_num+(counter/360.0), 1, 1);
        strip[i] = [obj.r, obj.g, obj.b];
      }
      break;
    case 1:
      for(let i = 0; i < led_num; i++){
        strip[i] = [solid_color.r, solid_color.g, solid_color.b];
      }
      break;
    case 2:
      breathing_counter += ((millis()-last_time)/1000)*0.1*2*Math.PI;
      multiplier = (1 + sin(breathing_counter)) / 2
      last_time = millis();
      for(let i = 0; i < led_num; i++){
        strip[i] = [solid_color.r * multiplier, solid_color.g * multiplier, solid_color.b * multiplier];
      }
      break;
      case 3:
        blinking_counter += ((millis()-last_time)/1000)*(config.anim_speed/100.0);
        last_time = millis();
        let offset = 1;
        if(blinking_counter < 0.5)
          offset = 1;
        else if(blinking_counter > 0.5){
          offset = 0;
        }
        if(blinking_counter > 1)
          blinking_counter = 0;
        for(let i = 0; i < led_num; i++){
          if((i + offset) % 2 == 0)
            strip[i] = [solid_color.r, solid_color.g, solid_color.b];
          else
            strip[i] = [18, 18, 18];
        }
        break;
      case 4:
        current_num += ((millis()-last_time)/1000)*2.0*60.0*(config.anim_speed/100.0);
        last_time = millis();
        if(current_num >= 60){
          current_num = 0;
          chase_step++;
          if(chase_step == 3)
            chase_step = 0;
        }
        for(let i = 0; i < led_num; i++){
          if(i == Math.floor(current_num)){
            strip[i] = [0, 0, 0];
            strip[i][chase_step] = config.brightness;
          }
          else
            strip[i] = [18, 18, 18];
        }
        break;
        default:
        for(let i = 0; i < led_num; i++){
          strip[i] = [18, 18, 18];
        }
      break;
  }

}

function setup() {
    var canvas = createCanvas(width, height);
    canvas.parent('canvas_container');
    last_time=millis();
  }
function draw() {
    clear(18,18,18);
    apply_animation();
    for(let i = 0; i < led_num; i++){
      fill(strip[i][0],strip[i][1],strip[i][2]);
      rect(i*(width/led_num), 0, (i+1)*(width/led_num), height);
    }
  }  