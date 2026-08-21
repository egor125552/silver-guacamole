const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const smooth=t=>t*t*(3-2*t);

function distanceGain(distance,refDistance,rolloff,maxDistance){
  const d=clamp(distance,refDistance,maxDistance);
  return refDistance/(refDistance+rolloff*(d-refDistance));
}

export default {
  id:"spatial-router",
  install(game){
    const c=game.audio.ctx;

    // One profile controls the whole 3D field. These values can be tuned live later
    // without changing any gameplay plugin.
    const profile={
      frontStereoDegrees:55,
      rearHrtfDegrees:120,
      frontHrtfFloor:.08,
      rearLowpassHz:9500,
      frontLowpassHz:20000,
    };
    game.audio.spatialProfile=profile;

    game.audio.setSpatializer((input,{name,position,relative,spatialMode="hybrid"})=>{
      // Listener-relative sounds still enter the same environment/reverb bus,
      // but stay centered because they have no world-space direction.
      if(relative)return input;

      const listener=game.state.player?.position||{x:0,z:0};
      const sourceY=position.y??game.audio.listenerHeight??1.65;
      const listenerY=game.audio.listenerHeight??1.65;
      const dx=position.x-listener.x;
      const dy=sourceY-listenerY;
      const dz=position.z-listener.z;
      const distance=Math.hypot(dx,dy,dz);
      const angle=Math.atan2(dx,-dz); // 0 = front, + = right, - = left
      const absDegrees=Math.abs(angle)*180/Math.PI;

      const front=profile.frontStereoDegrees;
      const rear=Math.max(front+1,profile.rearHrtfDegrees);
      const blend=smooth(clamp((absDegrees-front)/(rear-front),0,1));
      const hrtfMix=spatialMode==="hrtf"?1:(spatialMode==="stereo"?0:profile.frontHrtfFloor+(1-profile.frontHrtfFloor)*blend);
      const crossfade=hrtfMix*Math.PI*.5;
      const stereoWeight=Math.cos(crossfade);
      const hrtfWeight=Math.sin(crossfade);

      const refDistance=name==="EnemyPing"?11:4;
      const rolloff=name==="EnemyPing"?.62:1.5;
      const maxDistance=name==="EnemyPing"?60:10000;
      const range=c.createGain();
      range.gain.value=distanceGain(distance,refDistance,rolloff,maxDistance);
      input.connect(range);

      const stereoGain=c.createGain();
      stereoGain.gain.value=stereoWeight;
      const stereo=c.createStereoPanner();
      stereo.pan.value=clamp(Math.sin(angle),-1,1);
      range.connect(stereoGain);stereoGain.connect(stereo);

      const rearFilter=c.createBiquadFilter();
      rearFilter.type="lowpass";
      rearFilter.Q.value=.35;
      rearFilter.frequency.value=profile.frontLowpassHz-(profile.frontLowpassHz-profile.rearLowpassHz)*blend;
      const hrtfGain=c.createGain();
      hrtfGain.gain.value=hrtfWeight;
      const hrtf=c.createPanner();
      hrtf.panningModel="HRTF";
      hrtf.distanceModel="inverse";
      hrtf.refDistance=1;
      hrtf.rolloffFactor=0;
      hrtf.maxDistance=10000;
      if(hrtf.positionX){
        hrtf.positionX.value=position.x;hrtf.positionY.value=sourceY;hrtf.positionZ.value=position.z;
      }else hrtf.setPosition(position.x,sourceY,position.z);
      range.connect(rearFilter);rearFilter.connect(hrtfGain);hrtfGain.connect(hrtf);

      const mix=c.createGain();
      stereo.connect(mix);hrtf.connect(mix);
      return mix;
    });

    return()=>{
      game.audio.setSpatializer(null);
      delete game.audio.spatialProfile;
    };
  }
};
