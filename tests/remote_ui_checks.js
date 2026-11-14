// SPDX-License-Identifier: GPL-3.0-or-later
// Tests the browser transport UI. Native setup pages live in FirstBoot.qml.
const frame=document.querySelector('iframe'),results=document.getElementById('results'),lines=[];
const doc=()=>frame.contentDocument,$=id=>doc().getElementById(id),sleep=ms=>new Promise(r=>setTimeout(r,ms));
function assert(ok,message){if(!ok)throw new Error(message);lines.push('PASS '+message);results.textContent=lines.join('\n');}
async function until(predicate){for(let i=0;i<100;i++){if(predicate())return;await sleep(40);}throw new Error('Timed out waiting for UI');}
async function reset(body,width=1440,height=900){await fetch('/__test/reset',{method:'POST',body:JSON.stringify(body)});frame.style.width=width+'px';frame.style.height=height+'px';frame.src='/';await new Promise(r=>frame.onload=r);}
async function phase(value){await until(()=>doc().body.dataset.phase===value);await sleep(280);}
function fill(id,value){$(id).value=value;}
async function signIn(){fill('login-password','fixture-password');$('login-form').requestSubmit();}
try{
    await reset({});assert(!$('boot').hidden&&$('login').hidden,'Startup screen precedes sign-in');await phase('login');
    assert($('username-label').hidden,'First setup uses the provisioning password');
    assert(!$('wizard')&&!$('setup-desktop'),'No web installer or imitation desktop is present');
    fill('login-password','wrong');$('login-form').requestSubmit();await until(()=>!$('error').hidden);assert(!$('sign-in').disabled,'Incorrect password permits retry');
    await signIn();assert($('sign-in').disabled,'Sign-in prevents duplicate submissions');await phase('desktop');
    assert($('login-password').value==='','Password clears after sign-in');
    assert(!$('screen').hidden&&$('screen').children.length>0,'Setup login connects the RFB desktop');
    for(const [width,height]of [[1440,900],[1920,1080],[390,844],[600,500]]){
        frame.style.width=width+'px';frame.style.height=height+'px';await sleep(100);const r=$('screen').getBoundingClientRect();
        assert(r.x===0&&r.y===0&&Math.abs(r.width-width)<1&&Math.abs(r.height-height)<1,'Stream fills '+width+'×'+height);
    }
    assert(!$('desktop-tools')&&$('connection-panel').hidden,'Healthy desktop has no browser controls overlay');
    frame.style.width='1440px';frame.style.height='900px';
    let count=Number($('screen').dataset.connections);$('screen').dispatchEvent(new Event('test-disconnect'));
    await until(()=>!$('connection-panel').hidden);assert($('done').hidden&&$('boot').hidden,'Recovery never claims setup completion');
    await until(()=>Number($('screen').dataset.connections)>count&&$('connection-panel').hidden);assert($('error').hidden,'Transient loss reconnects automatically without another password');
    count=Number($('screen').dataset.connections);$('screen').dataset.failConnections='2';$('screen').dispatchEvent(new Event('test-disconnect'));
    await until(()=>Number($('screen').dataset.connections)>=count+3&&$('connection-panel').hidden);assert(true,'Failed reconnect attempts retry with backoff');
    await reset({role:'setup'});await phase('desktop');
    count=Number($('screen').dataset.connections);$('screen').dataset.failConnections='99';$('screen').dispatchEvent(new Event('test-disconnect'));
    await until(()=>!$('connection-panel').hidden&&!$('reconnect').hidden);await sleep(300);
    assert(Number($('screen').dataset.connections)===count+5,'Repeated failures stop after five retries');
    $('screen').dataset.failConnections='0';$('reconnect').click();await until(()=>$('connection-panel').hidden);assert($('error').hidden,'Manual recovery remains available after retries stop');
    const counters=async()=>await(await fetch('/__test/state')).json();
    await until(()=>$('connection-panel').hidden);await sleep(650);
    let pulses=(await counters()).keepalives;assert(pulses>0,'Active connection renews its authenticated session');
    const originalNow=frame.contentWindow.Date.now;frame.contentWindow.Date.now=()=>originalNow()+180000;await sleep(650);
    pulses=(await counters()).keepalives;await sleep(650);assert((await counters()).keepalives===pulses,'Idle desktop does not renew its session');
    frame.contentWindow.Date.now=originalNow;
    await reset({role:'setup'});await phase('desktop');assert($('login').hidden,'Setup session resumes directly to RFB');
    await fetch('/__test/reset',{method:'POST',body:JSON.stringify({mode:'desktop'})});
    $('screen').dispatchEvent(new Event('test-disconnect'));await phase('login');
    assert($('done').hidden&&!$('username-label').hidden,'Completed setup returns to normal sign-in without the legacy completion screen');
    await reset({mode:'desktop',cooldown:true});await phase('login');
    fill('login-user','  Maker  ');await signIn();await until(()=>!$('error').hidden);
    assert($('sign-in').disabled&&$('sign-in').textContent.includes('2'),'Rate limit shows a disabled countdown');
    assert((await counters()).username==='maker','Login canonicalizes account casing and whitespace');
    $('login-form').dispatchEvent(new Event('submit',{cancelable:true}));await sleep(200);
    assert((await counters()).loginRequests===1,'Cooldown never resubmits credentials');
    await until(()=>!$('sign-in').disabled);assert((await counters()).loginRequests===1,'Countdown expiry does not automatically retry');
    await signIn();await phase('desktop');assert(true,'Login recovers after cooldown');
    await reset({mode:'desktop'});await phase('login');assert(!$('username-label').hidden&&$('login-user').required,'Installed device requires local username');fill('login-user','maker');await signIn();await phase('desktop');
    assert($('screen').children.length>0,'Regular login also connects RFB');
    await fetch('/__test/reset',{method:'POST',body:JSON.stringify({mode:'desktop'})});$('screen').dispatchEvent(new Event('test-disconnect'));await phase('login');assert(!$('error').hidden,'Expired session returns to sign-in instead of retrying credentials');
    await reset({mode:'desktop',role:'desktop',renewal:false});await phase('desktop');await sleep(700);
    assert((await counters()).keepalives===0&&$('connection-panel').hidden,'Older running gateway remains usable until reboot');
    for(const [width,height]of [[390,844],[600,500]]){await reset({},width,height);await phase('login');const r=$('login').getBoundingClientRect();assert(r.left>=0&&r.right<=width+1,'Sign-in fits '+width+'×'+height);}
    results.textContent=lines.join('\n')+'\n\nALL '+lines.length+' CHECKS PASSED';document.title='PASS · PiOS browser checks';
}catch(error){results.textContent=lines.join('\n')+'\nFAIL '+error.stack;document.title='FAIL · PiOS browser checks';}
