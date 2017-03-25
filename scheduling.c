int
pseudoRandomGenerator(int maxNum){
  static unsigned int seed = 17;
  seed = (7359569 * seed + 4356783); 
  return seed  % maxNum;
}
