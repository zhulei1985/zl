export default function HashMap(){
    this.map = {};
}
HashMap.prototype = {
    put : function(key,value){
        this.map[key] = value;
    },
    get : function(key)
    {
        if (this.map.hasOwnProperty(key))
        {
            return this.map[key];
        }
        return null;
    },
    remove : function(key)
    {
        if (this.map.hasOwnProperty(key))
        {
            return delete this.map[key];
        }
        return false;
    },
    removeAll : function()
    {
        // TODO
    },
    keySet : function()
    {
        var _keys = [];
        for (var i in this.map)
        {
            _keys.push(i);
        }
        return _keys;
    }
};
