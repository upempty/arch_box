### shallow copy / deep copy  
Grid _grid = Grid(x, y);--shallow copy only copy the fla content which doesn't include the ptr allocated heap space.  
Grid *_grid = new Grid(x, y);
