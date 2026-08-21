export class InputState {
  constructor(bus){
    this.bus=bus;this.held=new Set();
    this.down=e=>{const key=this.normalize(e);if(this.isGameKey(key,e.code))e.preventDefault();if(!e.repeat)this.bus.emit("keyDown",{key,code:e.code});this.held.add(key);this.held.add(e.code);};
    this.up=e=>{const key=this.normalize(e);if(this.isGameKey(key,e.code))e.preventDefault();this.held.delete(key);this.held.delete(e.code);this.bus.emit("keyUp",{key,code:e.code});};
    this.blur=()=>this.held.clear();
  }
  normalize(e){if(e.key===" ")return"Space";return e.key.length===1?e.key.toLowerCase():e.key;}
  isGameKey(k,code){return["ArrowUp","ArrowDown","ArrowLeft","ArrowRight","Space","Shift","Control","Escape","w","a","s","d","x","f","e","b","r","1","2","3","4","5","6","7","8","9","0"].includes(k)||/^(Key[WASDXFEBR]|Digit[0-9]|Numpad[1-9]|Control(Left|Right)|Shift(Left|Right)|Space)$/.test(code);}
  attach(){addEventListener("keydown",this.down,{passive:false});addEventListener("keyup",this.up,{passive:false});addEventListener("blur",this.blur);}
  detach(){removeEventListener("keydown",this.down);removeEventListener("keyup",this.up);removeEventListener("blur",this.blur);}
  has(...keys){return keys.some(k=>this.held.has(k));}
}
