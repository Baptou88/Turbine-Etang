const allRangesv = document.querySelectorAll(".range-wrap-v");
allRangesv.forEach(wrap => {
  const range = wrap.querySelector(".range-v");
  const bubble = wrap.querySelector(".bubble-v");

  range.addEventListener("input", () => {
    setBubblev(range, bubble);
  });
  setBubblev(range, bubble);
});
function setBubblev(range, bubble) {
    const val = range.value;
    const min = range.min ? range.min : 0;
    const max = range.max ? range.max : 100;
    const w = range.width;
    const newVal = Number(((max-val) * 100) / (max - min));
    bubble.innerHTML = val + " " + newVal;
    
    // Sorta magic numbers based on size of the native UI thumb
    bubble.style.top = `calc(${newVal}% + (${-8 - newVal *.25 }px))`;
}