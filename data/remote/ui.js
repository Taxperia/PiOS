// SPDX-License-Identifier: GPL-3.0-or-later
const $=id=>document.getElementById(id);
const tr={starting:'PiOS başlatılıyor…',retry:'Tekrar dene',local:'YEREL MASAÜSTÜNÜZ',username:'Kullanıcı adı',password:'Parola',signin:'Devam et',logout:'Oturumu kapat',localConnection:'Sade bir alan. Tamamen sizin.',desktopTools:'Masaüstü kontrolleri',fullscreen:'Tam ekran',reconnect:'Yeniden bağlan',clipboard:'Pano',sendClipboard:'Metni masaüstüne gönder',complete:'Kurulum tamamlandı',restart:'Cihaz yeniden başlayacak. Ağına yeniden bağlanıp bu adresi tekrar açın. Tarayıcı erişimini açtıysanız yeni yerel hesabınızla giriş yapın.'};
const originals=new Map([...document.querySelectorAll('[data-i18n]')].map(node=>[node,node.textContent]));
tr.connectionReport='Bağlantı raporunu indir';
let language=navigator.language.startsWith('tr')?'tr':'en';
let mode='setup',role='',csrf='',booting=true,connecting=false,signingIn=false,completed=false,rfb=null,connectionTimer=null,desktopConnected=false;
// Retained for explicit legacy previews; the normal flow returns to account sign-in.
const legacyCompletionScreen=document.documentElement.dataset.legacyCompletionScreen==='true';
const text=(en,turkish)=>language==='tr'?turkish:en;
function error(message=''){$('error').hidden=!message;$('error').textContent=message;}
function status(message=''){$('connection').textContent=message;$('desktop-status').textContent=message;}
function updateClock(){const now=new Date();for(const clock of document.querySelectorAll('.clock'))clock.textContent=now.toLocaleTimeString(language,{hour:'2-digit',minute:'2-digit'});$('calendar-date').textContent=now.toLocaleDateString(language,{weekday:'long',day:'numeric',month:'long'});}
function translate(){$('language').value=language;document.documentElement.lang=language;for(const [node,original]of originals)node.textContent=language==='tr'?(tr[node.dataset.i18n]||original):original;updateClock();render();}
function render(){
    document.body.dataset.phase=booting?'boot':completed?'done':connecting?'connecting':role?'desktop':'login';
    $('boot').hidden=!booting&&!connecting;
    $('boot-status').textContent=connecting?text('Opening your desktop…','Masaüstünüz açılıyor…'):text('Starting PiOS…','PiOS başlatılıyor…');
    $('login').hidden=booting||!!role||completed;$('desktop').hidden=!role||completed;$('done').hidden=!completed;
    $('logout').hidden=!role||completed;
    $('connection-report').hidden=role!=='setup';
    $('login-title').textContent=mode==='setup'?text('Welcome to PiOS','PiOS’a hoş geldiniz'):text('Your PiOS desktop','PiOS masaüstünüz');
    $('login-help').textContent=mode==='setup'?text('Enter your one-time setup password to open the desktop and set up this device.','Masaüstünü açıp cihazı kurmak için tek kullanımlık kurulum parolanızı girin.'):text('Sign in with your local account.','Yerel kullanıcı hesabınızla giriş yapın.');
    $('username-label').hidden=mode==='setup';$('login-user').required=mode!=='setup';$('login-user').disabled=mode==='setup'||signingIn;
    $('sign-in').disabled=signingIn;$('sign-in').textContent=signingIn?text('Signing in…','Giriş yapılıyor…'):text('Continue','Devam et');
    $('fullscreen').textContent=document.fullscreenElement?text('Exit full screen','Tam ekrandan çık'):text('Full screen','Tam ekran');
    if(!role)status();
}
async function api(path,body){
    const response=await fetch(path,{method:body?'POST':'GET',headers:body?{'Content-Type':'application/json','X-CSRF-Token':csrf}:{},body:body?JSON.stringify(body):undefined,credentials:'same-origin',cache:'no-store',signal:AbortSignal.timeout(20000)});
    const result=await response.json();
    if(!response.ok){const message=result.error==='Sign-in failed'?text('Sign-in failed. Check your username and password.','Giriş başarısız. Kullanıcı adınızı ve parolanızı kontrol edin.'):result.error==='Sign in again'?text('Your session expired. Sign in again.','Oturumunuz sona erdi. Tekrar giriş yapın.'):result.error;const failure=new Error(message||text('Request failed. Retry shortly.','İşlem başarısız. Biraz sonra tekrar deneyin.'));failure.status=response.status;throw failure;}
    return result;
}
async function refresh(){const result=await api('/api/state');role=result.role;csrf=result.csrf;mode=result.mode||mode;render();}
function disconnect(){clearTimeout(connectionTimer);const client=rfb;rfb=null;client?.disconnect();connecting=false;desktopConnected=false;}
function signedOut(){disconnect();role='';csrf='';$('screen').replaceChildren();render();$(mode==='setup'?'login-password':'login-user').focus();}
function desktopFailure(message){disconnect();status(message);render();$('desktop-tools').open=true;error(message);}
async function disconnected(client){
    if(rfb!==client)return;
    rfb=null; // The transport already closed; do not disconnect it a second time.
    // The native installer owns completion/reboot. Never infer success just
    // from a broken stream; confirm a completed setup with the device first.
    desktopFailure(mode==='setup'?text('The setup desktop disconnected. If setup finished, the device is restarting. Otherwise reconnect to continue.','Kurulum masaüstü bağlantısı kesildi. Kurulumu bitirdiyseniz cihaz yeniden başlıyor. Aksi halde devam etmek için yeniden bağlanın.'):text('The desktop disconnected. Reconnect or sign in again.','Masaüstü bağlantısı kesildi. Yeniden bağlanın veya tekrar giriş yapın.'));
    if(mode==='setup')try{const result=await api('/api/mode');if(result.mode==='desktop'){
        mode='desktop';
        if(legacyCompletionScreen){completed=true;role='';render();}
        else signedOut();
        error();
    }}catch{}
}
async function connectDesktop(){
    disconnect();$('screen').replaceChildren();error();connecting=true;$('desktop-tools').open=false;render();
    connectionTimer=setTimeout(()=>desktopFailure(text('The desktop took too long to respond. Reconnect or sign in again.','Masaüstü yanıt vermedi. Yeniden bağlanın veya tekrar giriş yapın.')),30000);
    try{
        const {default:RFB}=await import('/novnc/core/rfb.js');if(!connecting)return;
        const url=new URL('/desktop-socket',location.href);url.protocol=location.protocol==='https:'?'wss:':'ws:';
        const client=new RFB($('screen'),url.href);rfb=client;client.scaleViewport=true;client.resizeSession=false;client.clipViewport=false;client.focusOnClick=true;
        client.addEventListener('connect',()=>{if(rfb!==client)return;clearTimeout(connectionTimer);connecting=false;desktopConnected=true;status(text('Connected to your PiOS desktop','PiOS masaüstünüze bağlandı'));render();client.focus();});
        client.addEventListener('disconnect',()=>{void disconnected(client);});
        client.addEventListener('securityfailure',()=>{if(rfb===client)desktopFailure(text('Desktop authentication failed. Sign in again.','Masaüstü doğrulaması başarısız. Tekrar giriş yapın.'));});
        client.addEventListener('clipboard',event=>{if(rfb===client)$('clipboard').value=event.detail.text;});
    }catch{desktopFailure(text('The desktop could not open. Reconnect or sign in again.','Masaüstü açılamadı. Yeniden bağlanın veya tekrar giriş yapın.'));}
}
function handle(operation){return async event=>{event?.preventDefault();error();try{await operation();}catch(e){if(role&&(e.status===401||e.status===403))signedOut();error(e.message||text('The connection changed. Reconnect to the device and retry.','Bağlantı değişti. Cihaza yeniden bağlanıp tekrar deneyin.'));}};}
async function signIn(){
    if(signingIn)return;signingIn=true;render();
    try{const result=await api('/api/login',{username:$('login-user').value,password:$('login-password').value});$('login-password').value='';role=result.role;csrf=result.csrf;await refresh();await connectDesktop();}finally{signingIn=false;render();}
}
const signOut=handle(async()=>{try{await api('/api/logout',{});}catch(e){if(e.status!==401&&e.status!==403)throw e;}signedOut();location.reload();});
$('login-form').addEventListener('submit',handle(signIn));
$('logout').addEventListener('click',signOut);$('desktop-logout').addEventListener('click',signOut);
$('language').addEventListener('change',()=>{language=$('language').value;translate();});
$('connection-report').addEventListener('click',handle(async()=>{
    const report=await api('/api/diagnostics');
    const url=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)],{type:'application/json'}));
    const link=document.createElement('a');link.href=url;link.download='pios-connection-report.json';link.click();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
}));
$('reconnect').addEventListener('click',handle(async()=>{await refresh();await connectDesktop();}));
$('fullscreen').addEventListener('click',handle(async()=>{if(document.fullscreenElement)await document.exitFullscreen();else await document.documentElement.requestFullscreen();$('desktop-tools').open=false;}));
// Keep the canvas mounted while toggling full screen; noVNC observes its size.
 document.addEventListener('fullscreenchange',render);
$('send-clipboard').addEventListener('click',handle(async()=>{if(!desktopConnected)throw new Error(text('Reconnect to the desktop first.','Önce masaüstüne yeniden bağlanın.'));rfb.clipboardPasteFrom($('clipboard').value);}));
async function start(){
    booting=true;completed=false;$('retry-start').hidden=true;error();translate();
    try{mode=(await api('/api/mode')).mode;try{await refresh();}catch(e){if(e.status!==401)throw e;role='';}booting=false;render();if(role)await connectDesktop();else $(mode==='setup'?'login-password':'login-user').focus();}
    catch(e){error(e.message);$('retry-start').hidden=false;}
}
$('retry-start').addEventListener('click',start);setInterval(updateClock,1000);start();
