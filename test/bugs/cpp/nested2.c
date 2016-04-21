#define HERE_BE_DRAGONS 1




#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#define A                                           \
	JOIN(H,                                           \
		JOIN(E,                                         \
			JOIN(R,                                       \
				JOIN(E,                                     \
					JOIN(_,                                   \
						JOIN(B,                                 \
							JOIN(E,                               \
								JOIN(_,                             \
									JOIN(D,                           \
										JOIN(R,                         \
											JOIN(A,                       \
												JOIN(G,                     \
													JOIN(O,                   \
														JOIN(N, S))))))))))))))


#if A
dragons!
#else
no dragons
#endif
