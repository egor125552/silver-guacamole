export const CampaignStages=Object.freeze([
  {id:"stash",title:"Тайник",text:"Доберитесь до тайника в старом хозяйственном блоке.",target:{x:-38,z:-22}},
  {id:"key",title:"Ключевой охранник",text:"Найдите старшего охранника и заберите у него ключ от внутренней двери.",target:{x:34,z:-32}},
  {id:"control",title:"Пульт тревоги",text:"Доберитесь до пульта и выдержите тревогу.",target:{x:42,z:18}},
  {id:"yard",title:"Прогулочный двор",text:"Найдите стрелка, который перекрывает путь через двор.",target:{x:-48,z:42}},
  {id:"armory",title:"Оружейная",text:"Пробейтесь к оружейной и устраните капитана охраны.",target:{x:55,z:54}},
  {id:"generator",title:"Генератор",text:"Доберитесь до генератора и удерживайте сектор, пока питание отключается.",target:{x:4,z:74}},
  {id:"warden",title:"Начальник блока",text:"Устраните начальника блока и его охрану.",target:{x:-62,z:68}},
  {id:"exit",title:"Выход",text:"Доберитесь до внешних ворот и переживите финальную блокировку.",target:{x:-90,z:90}},
]);
function clearAround(walls,target,radius=8){return walls.filter(w=>{const cx=w.x+w.w*.5,cz=w.z+w.h*.5;return Math.hypot(cx-target.x,cz-target.z)>radius+Math.hypot(w.w,w.h)*.5;});}
export default{id:"campaign-state",install(game){game.campaignStage=()=>CampaignStages[game.state.campaign?.stage??0]||null;game.campaignObjectivePosition=()=>{const c=game.state.campaign;if(!c)return null;if(c.objectiveEnemyId!=null){const e=game.state.enemies.find(x=>x.id===c.objectiveEnemyId&&x.alive);if(e)return e.position;}return game.campaignStage()?.target||null;};game.campaignDescribe=()=>{const c=game.state.campaign,s=game.campaignStage();if(!c||!s)return c?.completed?"Побег завершён.":"";let extra="";if(c.holdUntil>game.time)extra=` Осталось удерживать позицию ${Math.ceil(c.holdUntil-game.time)} секунд.`;if(s.id==="exit"&&game.time<c.minimumVictoryTime)extra=` Ворота заблокированы. До открытия ${Math.ceil(c.minimumVictoryTime-game.time)} секунд. Держитесь.`;return `Этап ${c.stage+1} из ${CampaignStages.length}: ${s.text}${extra}`;};return game.on("reset",()=>{for(const stage of CampaignStages)game.state.walls=clearAround(game.state.walls,stage.target,9);game.state.campaign={active:true,stage:0,stageSince:0,objectiveEnemyId:null,holdUntil:0,nextWaveAt:0,wave:0,completed:false,minimumVictoryTime:20*60};game.state.enemies=game.state.enemies.filter((e,i)=>i<16);});}};
