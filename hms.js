let PORT = 8742;
let PORT_WS = 8743;
//appends to site url
let URL = (s)=>(`http://0.0.0.0:${PORT}/${s}`);
let URL_WS = (s)=>(`ws://0.0.0.0:${PORT_WS}/${s}`);

let readfile = async (path,retText=true)=>{
    let resp = await fetch(URL(`readfile/${path}`),
        {method:"GET"});
    if(retText) return (await resp.text());
    else return (await resp);
};
let ls = async (path,removeQuotes=true)=>{
    let resp = await fetch( URL(`ls/${path}`) ,
        {method:"GET"});
    let txt = (await resp.text()).split('\n');
    txt.pop(); // remove empty line
    if(removeQuotes)
        txt = txt.map(s=>s.substring(1,s.length-1));
    return txt;
};