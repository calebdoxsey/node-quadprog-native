var quadprog = require("./build/Release/quadprog");

console.log(
  quadprog.solve(
    [[1,0,0,0],
     [0,7,0,0],
     [0,0,1,0],
     [0,0,0,1]
    ],
    [0.25,0.25,0.25,0.25],

    [],
    [],

    [[1,0,0,0],
     [0,1,0,0],
     [0,0,1,0],
     [0,0,0,1]
    ],
    [0,0,0,0]
  )
);