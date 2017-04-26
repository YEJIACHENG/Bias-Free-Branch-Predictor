# Bias-Free-Branch-Predictor
Practical implementation of Bias Free Branch Predictor proposed by Dibakar Gope and Mikko H. Lipasti. 2014. Bias-Free Branch Predictor. In Proceedings of the 47th Annual IEEE/ACM International Symposium on Microarchitecture (MICRO-47). IEEE Computer Society, Washington, DC, USA, 521-532. DOI: http://dx.doi.org/10.1109/MICRO.2014.32



Pipeline:


+ Global History Register: GHR contains only recent occurence of non-biased branches as they are executed

+ Branch Status Table: BST

+ A: array of address of the non-biased branches

+ P: The absolute distance in the past global history of corresponding non-biased branches included in array A. P captures the pos hist of the non-biased branches present in the RS.

+ Wb, Wm: one-dimensional and two-dimensional arrays of integer weights respectively. 
  - Wb is the bias weight table,
  - Wm is the correlating weight table.

+ Training Algorithm 3 :
  - Input:
    BST - Branch Status Table
  - Ouput:
    W_b, W_m, W_rs, RS, updated GHR

+ BF-Neural Prediction Algorithm 2
  - Input:
    BST, W_b, W_m, W_rs,RS, GHR, A
  - Output:
    prediction: taken ? not_taken
