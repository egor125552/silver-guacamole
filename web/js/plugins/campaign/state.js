import {Weapon,AIBehavior} from "../../core/constants.js";

export const CampaignStages=Object.freeze([
  {id:"stash",title:"Тайник",difficulty:"Легко",text:"Доберитесь до тайника в старом хозяйственном блоке.",hint:"Сейчас у вас только кулаки. На первом этапе у врагов нет огнестрела: не обязаны драться со всеми, используйте сонар и обходите патрули.",target:{x:-38,z:-22}},
  {id:"key",title:"Ключевой охранник",difficulty:"Легко",text:"Найдите старшего охранника и заберите у него ключ от внутренней двери.",hint:"После тайника у вас есть заточка. Ключевой охранник вооружён только дубинкой.",target:{x:34,z:-32}},
  {id:"control",title:"Пульт тревоги",difficulty:"Средне",text:"Доберитесь до пульта и выдержите тревогу.",hint:"К этому моменту уже открыт пистолет. Волны небольшие: сначала ближний бой, затем один стрелок за раз.",target:{x:42,z:18}},
  {id:"yard",title:"Прогулочный двор",difficulty:"Средне",text:"Найдите стрелка, который перекрывает путь через двор.",hint:"Во дворе один основной стрелок и один помощник. Пистолет у вас уже есть.",target:{x:-48,z:42}},
  {id:"armory",title:"Оружейная",difficulty:"Сложно",text:"Пробейтесь к оружейной и устраните капитана охраны.",hint:"Перед оружейной открывается автомат. Капитан тоже с автоматом, но его прикрывают только двое.",target:{x:55,z:54}},
  {id:"generator",title:"Генератор",difficulty:"Сложно",text:"Доберитесь до генератора и удерживайте сектор, пока питание отключается.",hint:"Подкрепления приходят редко и небольшими группами. Между волнами есть время восстановиться.",target:{x:4,z:74}},
  {id:"warden",title:"Начальник блока",difficulty:"Очень сложно",text:"Устраните начальника блока и его охрану.",hint:"Снайперская винтовка уже открыта. Это первый этап, где одновременно встречается несколько вооружённых противников.",target:{x:-62,z:68}},
  {id:"exit",title:"Выход",difficulty:"Финал",text:"Доберитесь до внешних ворот и переживите финальную блокировку.",hint:"К финалу открыты все 11 видов оружия. Подкрепления ограничены и приходят с паузами.",target:{x:-90,z:90}},
]);

function clearAround(walls,target,radius=8){
  return walls.filter(w=>{
    const cx=w.x+w.w*.5,cz=w.z+w.h*.5;
    return Math.hypot(cx-target.x,cz-target.z)>radius+Math.hypot(w.w,w.h)*.5;
  });
}

function prepareOpeningPopulation(game){
  const roles=["idle","patroller","investigator","sentry","patroller","idle","charger","sentry"];
  game.state.enemies=game.state.enemies.slice(0,8);
  for(let i=0;i<game.state.enemies.length;i++){
    const e=game.state.enemies[i];
    e.role=roles[i];
    e.weapon=i%3===0?Weapon.BATON:Weapon.FIST;
    e.behavior=AIBehavior.AGGRESSOR;
    e.health=e.maxHealth=Math.min(e.maxHealth,i%3===0?130:105);
    e.waiting=e.role==="idle"||e.role==="sentry";
    e.detection=0;
  }
}

export default{
  id:"campaign-state",
  install(game){
    game.campaignStage=()=>CampaignStages[game.state.campaign?.stage??0]||null;
    game.campaignObjectivePosition=()=>{
      const c=game.state.campaign;if(!c)return null;
      if(c.objectiveEnemyId!=null){
        const e=game.state.enemies.find(x=>x.id===c.objectiveEnemyId&&x.alive);
        if(e)return e.position;
      }
      return game.campaignStage()?.target||null;
    };
    game.campaignDescribe=()=>{
      const c=game.state.campaign,s=game.campaignStage();
      if(!c||!s)return c?.completed?"Побег завершён.":"";
      let extra="";
      if(c.holdUntil>game.time)extra=` Осталось удерживать позицию ${Math.ceil(c.holdUntil-game.time)} секунд.`;
      if(s.id==="exit"&&game.time<c.minimumVictoryTime)extra=` Ворота заблокированы. До открытия ${Math.ceil(c.minimumVictoryTime-game.time)} секунд. Держитесь.`;
      const arsenal=game.state.player?.unlockedWeapons&&game.weaponName?` Доступное оружие: ${[...game.state.player.unlockedWeapons].map(game.weaponName).join(", ")}.`:"";
      return `Этап ${c.stage+1} из ${CampaignStages.length}. Сложность: ${s.difficulty}. ${s.text} Подсказка: ${s.hint}${arsenal}${extra}`;
    };
    return game.on("reset",()=>{
      for(const stage of CampaignStages)game.state.walls=clearAround(game.state.walls,stage.target,9);
      game.state.campaign={active:true,stage:0,stageSince:0,objectiveEnemyId:null,holdUntil:0,nextWaveAt:0,wave:0,completed:false,minimumVictoryTime:20*60};
      prepareOpeningPopulation(game);
    });
  }
};
