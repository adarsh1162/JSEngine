let obj = {
    value: 42,
    getValue: function() {
        return this.value;
    }
};
console.log("Value from this: " + obj.getValue());
