
build/init:     file format elf64-x86-64


Disassembly of section .note.gnu.build-id:

0000000000200158 <.note.gnu.build-id>:
  200158:	04 00                	add    $0x0,%al
  20015a:	00 00                	add    %al,(%rax)
  20015c:	14 00                	adc    $0x0,%al
  20015e:	00 00                	add    %al,(%rax)
  200160:	03 00                	add    (%rax),%eax
  200162:	00 00                	add    %al,(%rax)
  200164:	47                   	rex.RXB
  200165:	4e 55                	rex.WRX push %rbp
  200167:	00 37                	add    %dh,(%rdi)
  200169:	da e6                	(bad)
  20016b:	59                   	pop    %rcx
  20016c:	94                   	xchg   %eax,%esp
  20016d:	70 ea                	jo     200159 <strlen-0x10b7>
  20016f:	0f 0b                	ud2
  200171:	33 f1                	xor    %ecx,%esi
  200173:	c8 c4 0d 11          	enter  $0xdc4,$0x11
  200177:	cc                   	int3
  200178:	47 29 19             	rex.RXB sub %r11d,(%r9)
  20017b:	6c                   	insb   (%dx),%es:(%rdi)

Disassembly of section .rodata:

000000000020017c <.rodata>:
  20017c:	55                   	push   %rbp
  20017d:	73 65                	jae    2001e4 <strlen-0x102c>
  20017f:	72 73                	jb     2001f4 <strlen-0x101c>
  200181:	00 47 75             	add    %al,0x75(%rdi)
  200184:	65 73 74             	gs jae 2001fb <strlen-0x1015>
  200187:	00 6d 6f             	add    %ch,0x6f(%rbp)
  20018a:	74 64                	je     2001f0 <strlen-0x1020>
  20018c:	2e 74 78             	je,pn  200207 <strlen-0x1009>
  20018f:	74 00                	je     200191 <strlen-0x107f>
  200191:	46 61                	rex.RX (bad)
  200193:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  20019a:	20 
  20019b:	6f                   	outsl  %ds:(%rsi),(%dx)
  20019c:	70 65                	jo     200203 <strlen-0x100d>
  20019e:	6e                   	outsb  %ds:(%rsi),(%dx)
  20019f:	20 66 69             	and    %ah,0x69(%rsi)
  2001a2:	6c                   	insb   (%dx),%es:(%rdi)
  2001a3:	65 20 2f             	and    %ch,%gs:(%rdi)
  2001a6:	55                   	push   %rbp
  2001a7:	73 65                	jae    20020e <strlen-0x1002>
  2001a9:	72 73                	jb     20021e <strlen-0xff2>
  2001ab:	2f                   	(bad)
  2001ac:	47 75 65             	rex.RXB jne 200214 <strlen-0xffc>
  2001af:	73 74                	jae    200225 <strlen-0xfeb>
  2001b1:	2f                   	(bad)
  2001b2:	6d                   	insl   (%dx),%es:(%rdi)
  2001b3:	6f                   	outsl  %ds:(%rsi),(%dx)
  2001b4:	74 64                	je     20021a <strlen-0xff6>
  2001b6:	2e 74 78             	je,pn  200231 <strlen-0xfdf>
  2001b9:	74 00                	je     2001bb <strlen-0x1055>
  2001bb:	52                   	push   %rdx
  2001bc:	65 61                	gs (bad)
  2001be:	64 69 6e 67 20 66 69 	imul   $0x6c696620,%fs:0x67(%rsi),%ebp
  2001c5:	6c 
  2001c6:	65 20 2f             	and    %ch,%gs:(%rdi)
  2001c9:	55                   	push   %rbp
  2001ca:	73 65                	jae    200231 <strlen-0xfdf>
  2001cc:	72 73                	jb     200241 <strlen-0xfcf>
  2001ce:	2f                   	(bad)
  2001cf:	47 75 65             	rex.RXB jne 200237 <strlen-0xfd9>
  2001d2:	73 74                	jae    200248 <strlen-0xfc8>
  2001d4:	2f                   	(bad)
  2001d5:	6d                   	insl   (%dx),%es:(%rdi)
  2001d6:	6f                   	outsl  %ds:(%rsi),(%dx)
  2001d7:	74 64                	je     20023d <strlen-0xfd3>
  2001d9:	2e 74 78             	je,pn  200254 <strlen-0xfbc>
  2001dc:	74 00                	je     2001de <strlen-0x1032>
  2001de:	46 61                	rex.RX (bad)
  2001e0:	69 6c 65 64 20 74 6f 	imul   $0x206f7420,0x64(%rbp,%riz,2),%ebp
  2001e7:	20 
  2001e8:	72 65                	jb     20024f <strlen-0xfc1>
  2001ea:	61                   	(bad)
  2001eb:	64 20 66 69          	and    %ah,%fs:0x69(%rsi)
  2001ef:	6c                   	insb   (%dx),%es:(%rdi)
  2001f0:	65 20 2f             	and    %ch,%gs:(%rdi)
  2001f3:	55                   	push   %rbp
  2001f4:	73 65                	jae    20025b <strlen-0xfb5>
  2001f6:	72 73                	jb     20026b <strlen-0xfa5>
  2001f8:	2f                   	(bad)
  2001f9:	47 75 65             	rex.RXB jne 200261 <strlen-0xfaf>
  2001fc:	73 74                	jae    200272 <strlen-0xf9e>
  2001fe:	2f                   	(bad)
  2001ff:	6d                   	insl   (%dx),%es:(%rdi)
  200200:	6f                   	outsl  %ds:(%rsi),(%dx)
  200201:	74 64                	je     200267 <strlen-0xfa9>
  200203:	2e 74 78             	je,pn  20027e <strlen-0xf92>
  200206:	74 00                	je     200208 <strlen-0x1008>

Disassembly of section .text:

0000000000201210 <strlen>:
  201210:	55                   	push   %rbp
  201211:	48 89 e5             	mov    %rsp,%rbp
  201214:	48 83 ec 10          	sub    $0x10,%rsp
  201218:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20121c:	48 c7 45 f0 00 00 00 	movq   $0x0,-0x10(%rbp)
  201223:	00 
  201224:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  201228:	48 89 c1             	mov    %rax,%rcx
  20122b:	48 83 c1 01          	add    $0x1,%rcx
  20122f:	48 89 4d f8          	mov    %rcx,-0x8(%rbp)
  201233:	80 38 00             	cmpb   $0x0,(%rax)
  201236:	74 0e                	je     201246 <strlen+0x36>
  201238:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  20123c:	48 83 c0 01          	add    $0x1,%rax
  201240:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  201244:	eb de                	jmp    201224 <strlen+0x14>
  201246:	48 8b 45 f0          	mov    -0x10(%rbp),%rax
  20124a:	48 83 c4 10          	add    $0x10,%rsp
  20124e:	5d                   	pop    %rbp
  20124f:	c3                   	ret

0000000000201250 <ClientStart>:
  201250:	55                   	push   %rbp
  201251:	48 89 e5             	mov    %rsp,%rbp
  201254:	48 81 ec 40 01 00 00 	sub    $0x140,%rsp
  20125b:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20125f:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  201263:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  201267:	48 c7 c0 ff ff ff ff 	mov    $0xffffffffffffffff,%rax
  20126e:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  201272:	48 8d 3d 03 ef ff ff 	lea    -0x10fd(%rip),%rdi        # 20017c <strlen-0x1094>
  201279:	48 8d 75 e0          	lea    -0x20(%rbp),%rsi
  20127d:	e8 9e 00 00 00       	call   201320 <_ZL8OpenFileILm21EEmRAT__KcPP18OsFileHandleHandle>
  201282:	48 89 45 d8          	mov    %rax,-0x28(%rbp)
  201286:	48 83 7d d8 00       	cmpq   $0x0,-0x28(%rbp)
  20128b:	74 0e                	je     20129b <ClientStart+0x4b>
  20128d:	48 8d 3d fd ee ff ff 	lea    -0x1103(%rip),%rdi        # 200191 <strlen-0x107f>
  201294:	e8 d7 00 00 00       	call   201370 <_ZL10OsDebugLogPKc>
  201299:	eb fe                	jmp    201299 <ClientStart+0x49>
  20129b:	48 c7 85 c8 fe ff ff 	movq   $0xffffffffffffffff,-0x138(%rbp)
  2012a2:	ff ff ff ff 
  2012a6:	48 8b 7d e0          	mov    -0x20(%rbp),%rdi
  2012aa:	48 8d b5 d0 fe ff ff 	lea    -0x130(%rbp),%rsi
  2012b1:	48 8d 95 d0 fe ff ff 	lea    -0x130(%rbp),%rdx
  2012b8:	48 81 c2 00 01 00 00 	add    $0x100,%rdx
  2012bf:	48 8d 8d c8 fe ff ff 	lea    -0x138(%rbp),%rcx
  2012c6:	e8 d5 01 00 00       	call   2014a0 <OsFileRead>
  2012cb:	48 89 85 c0 fe ff ff 	mov    %rax,-0x140(%rbp)
  2012d2:	48 83 bd c0 fe ff ff 	cmpq   $0x0,-0x140(%rbp)
  2012d9:	00 
  2012da:	74 0e                	je     2012ea <ClientStart+0x9a>
  2012dc:	48 8d 3d fb ee ff ff 	lea    -0x1105(%rip),%rdi        # 2001de <strlen-0x1032>
  2012e3:	e8 88 00 00 00       	call   201370 <_ZL10OsDebugLogPKc>
  2012e8:	eb fe                	jmp    2012e8 <ClientStart+0x98>
  2012ea:	48 8d 3d ca ee ff ff 	lea    -0x1136(%rip),%rdi        # 2001bb <strlen-0x1055>
  2012f1:	e8 7a 00 00 00       	call   201370 <_ZL10OsDebugLogPKc>
  2012f6:	48 8d bd d0 fe ff ff 	lea    -0x130(%rbp),%rdi
  2012fd:	48 8d b5 d0 fe ff ff 	lea    -0x130(%rbp),%rsi
  201304:	48 03 b5 c8 fe ff ff 	add    -0x138(%rbp),%rsi
  20130b:	e8 b0 00 00 00       	call   2013c0 <_ZL10OsDebugLogPKcS0_>
  201310:	eb fe                	jmp    201310 <ClientStart+0xc0>
  201312:	cc                   	int3
  201313:	cc                   	int3
  201314:	cc                   	int3
  201315:	cc                   	int3
  201316:	cc                   	int3
  201317:	cc                   	int3
  201318:	cc                   	int3
  201319:	cc                   	int3
  20131a:	cc                   	int3
  20131b:	cc                   	int3
  20131c:	cc                   	int3
  20131d:	cc                   	int3
  20131e:	cc                   	int3
  20131f:	cc                   	int3

0000000000201320 <_ZL8OpenFileILm21EEmRAT__KcPP18OsFileHandleHandle>:
  201320:	55                   	push   %rbp
  201321:	48 89 e5             	mov    %rsp,%rbp
  201324:	48 83 ec 20          	sub    $0x20,%rsp
  201328:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20132c:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  201330:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  201334:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  201338:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  20133c:	48 83 c0 15          	add    $0x15,%rax
  201340:	48 83 c0 ff          	add    $0xffffffffffffffff,%rax
  201344:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  201348:	48 8b 7d e8          	mov    -0x18(%rbp),%rdi
  20134c:	48 8b 75 e0          	mov    -0x20(%rbp),%rsi
  201350:	48 8b 4d f0          	mov    -0x10(%rbp),%rcx
  201354:	ba 01 00 00 00       	mov    $0x1,%edx
  201359:	e8 a2 00 00 00       	call   201400 <OsFileOpen>
  20135e:	48 83 c4 20          	add    $0x20,%rsp
  201362:	5d                   	pop    %rbp
  201363:	c3                   	ret
  201364:	cc                   	int3
  201365:	cc                   	int3
  201366:	cc                   	int3
  201367:	cc                   	int3
  201368:	cc                   	int3
  201369:	cc                   	int3
  20136a:	cc                   	int3
  20136b:	cc                   	int3
  20136c:	cc                   	int3
  20136d:	cc                   	int3
  20136e:	cc                   	int3
  20136f:	cc                   	int3

0000000000201370 <_ZL10OsDebugLogPKc>:
  201370:	55                   	push   %rbp
  201371:	48 89 e5             	mov    %rsp,%rbp
  201374:	48 83 ec 20          	sub    $0x20,%rsp
  201378:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20137c:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  201380:	48 89 45 f0          	mov    %rax,-0x10(%rbp)
  201384:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
  201388:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  20138c:	48 8b 7d f8          	mov    -0x8(%rbp),%rdi
  201390:	e8 7b fe ff ff       	call   201210 <strlen>
  201395:	48 89 c1             	mov    %rax,%rcx
  201398:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  20139c:	48 01 c8             	add    %rcx,%rax
  20139f:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  2013a3:	48 8b 7d f0          	mov    -0x10(%rbp),%rdi
  2013a7:	48 8b 75 e8          	mov    -0x18(%rbp),%rsi
  2013ab:	e8 10 00 00 00       	call   2013c0 <_ZL10OsDebugLogPKcS0_>
  2013b0:	48 83 c4 20          	add    $0x20,%rsp
  2013b4:	5d                   	pop    %rbp
  2013b5:	c3                   	ret
  2013b6:	cc                   	int3
  2013b7:	cc                   	int3
  2013b8:	cc                   	int3
  2013b9:	cc                   	int3
  2013ba:	cc                   	int3
  2013bb:	cc                   	int3
  2013bc:	cc                   	int3
  2013bd:	cc                   	int3
  2013be:	cc                   	int3
  2013bf:	cc                   	int3

00000000002013c0 <_ZL10OsDebugLogPKcS0_>:
  2013c0:	55                   	push   %rbp
  2013c1:	48 89 e5             	mov    %rsp,%rbp
  2013c4:	48 83 ec 20          	sub    $0x20,%rsp
  2013c8:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  2013cc:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  2013d0:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  2013d4:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  2013d8:	31 c0                	xor    %eax,%eax
  2013da:	31 c0                	xor    %eax,%eax
  2013dc:	41 89 c0             	mov    %eax,%r8d
  2013df:	bf 90 00 00 00       	mov    $0x90,%edi
  2013e4:	4c 89 c1             	mov    %r8,%rcx
  2013e7:	e8 18 02 00 00       	call   201604 <OsSystemCall>
  2013ec:	48 89 45 e0          	mov    %rax,-0x20(%rbp)
  2013f0:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  2013f4:	48 83 c4 20          	add    $0x20,%rsp
  2013f8:	5d                   	pop    %rbp
  2013f9:	c3                   	ret
  2013fa:	cc                   	int3
  2013fb:	cc                   	int3
  2013fc:	cc                   	int3
  2013fd:	cc                   	int3
  2013fe:	cc                   	int3
  2013ff:	cc                   	int3

0000000000201400 <OsFileOpen>:
  201400:	55                   	push   %rbp
  201401:	48 89 e5             	mov    %rsp,%rbp
  201404:	48 83 ec 30          	sub    $0x30,%rsp
  201408:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20140c:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  201410:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  201414:	48 89 4d e0          	mov    %rcx,-0x20(%rbp)
  201418:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  20141c:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  201420:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  201424:	31 c0                	xor    %eax,%eax
  201426:	31 c0                	xor    %eax,%eax
  201428:	41 89 c0             	mov    %eax,%r8d
  20142b:	bf 10 00 00 00       	mov    $0x10,%edi
  201430:	e8 cf 01 00 00       	call   201604 <OsSystemCall>
  201435:	48 89 45 d0          	mov    %rax,-0x30(%rbp)
  201439:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  20143d:	48 8b 4d d8          	mov    -0x28(%rbp),%rcx
  201441:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  201445:	48 89 08             	mov    %rcx,(%rax)
  201448:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  20144c:	48 83 c4 30          	add    $0x30,%rsp
  201450:	5d                   	pop    %rbp
  201451:	c3                   	ret
  201452:	cc                   	int3
  201453:	cc                   	int3
  201454:	cc                   	int3
  201455:	cc                   	int3
  201456:	cc                   	int3
  201457:	cc                   	int3
  201458:	cc                   	int3
  201459:	cc                   	int3
  20145a:	cc                   	int3
  20145b:	cc                   	int3
  20145c:	cc                   	int3
  20145d:	cc                   	int3
  20145e:	cc                   	int3
  20145f:	cc                   	int3

0000000000201460 <OsFileClose>:
  201460:	55                   	push   %rbp
  201461:	48 89 e5             	mov    %rsp,%rbp
  201464:	48 83 ec 20          	sub    $0x20,%rsp
  201468:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20146c:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  201470:	31 c0                	xor    %eax,%eax
  201472:	31 c0                	xor    %eax,%eax
  201474:	41 89 c0             	mov    %eax,%r8d
  201477:	bf 11 00 00 00       	mov    $0x11,%edi
  20147c:	4c 89 c2             	mov    %r8,%rdx
  20147f:	4c 89 c1             	mov    %r8,%rcx
  201482:	e8 7d 01 00 00       	call   201604 <OsSystemCall>
  201487:	48 89 45 e8          	mov    %rax,-0x18(%rbp)
  20148b:	48 89 55 f0          	mov    %rdx,-0x10(%rbp)
  20148f:	48 8b 45 e8          	mov    -0x18(%rbp),%rax
  201493:	48 83 c4 20          	add    $0x20,%rsp
  201497:	5d                   	pop    %rbp
  201498:	c3                   	ret
  201499:	cc                   	int3
  20149a:	cc                   	int3
  20149b:	cc                   	int3
  20149c:	cc                   	int3
  20149d:	cc                   	int3
  20149e:	cc                   	int3
  20149f:	cc                   	int3

00000000002014a0 <OsFileRead>:
  2014a0:	55                   	push   %rbp
  2014a1:	48 89 e5             	mov    %rsp,%rbp
  2014a4:	48 83 ec 30          	sub    $0x30,%rsp
  2014a8:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  2014ac:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  2014b0:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  2014b4:	48 89 4d e0          	mov    %rcx,-0x20(%rbp)
  2014b8:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  2014bc:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  2014c0:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  2014c4:	31 c0                	xor    %eax,%eax
  2014c6:	31 c0                	xor    %eax,%eax
  2014c8:	41 89 c0             	mov    %eax,%r8d
  2014cb:	bf 12 00 00 00       	mov    $0x12,%edi
  2014d0:	e8 2f 01 00 00       	call   201604 <OsSystemCall>
  2014d5:	48 89 45 d0          	mov    %rax,-0x30(%rbp)
  2014d9:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  2014dd:	48 8b 4d d8          	mov    -0x28(%rbp),%rcx
  2014e1:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  2014e5:	48 89 08             	mov    %rcx,(%rax)
  2014e8:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  2014ec:	48 83 c4 30          	add    $0x30,%rsp
  2014f0:	5d                   	pop    %rbp
  2014f1:	c3                   	ret
  2014f2:	cc                   	int3
  2014f3:	cc                   	int3
  2014f4:	cc                   	int3
  2014f5:	cc                   	int3
  2014f6:	cc                   	int3
  2014f7:	cc                   	int3
  2014f8:	cc                   	int3
  2014f9:	cc                   	int3
  2014fa:	cc                   	int3
  2014fb:	cc                   	int3
  2014fc:	cc                   	int3
  2014fd:	cc                   	int3
  2014fe:	cc                   	int3
  2014ff:	cc                   	int3

0000000000201500 <OsFileWrite>:
  201500:	55                   	push   %rbp
  201501:	48 89 e5             	mov    %rsp,%rbp
  201504:	48 83 ec 30          	sub    $0x30,%rsp
  201508:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20150c:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  201510:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  201514:	48 89 4d e0          	mov    %rcx,-0x20(%rbp)
  201518:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  20151c:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  201520:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  201524:	31 c0                	xor    %eax,%eax
  201526:	31 c0                	xor    %eax,%eax
  201528:	41 89 c0             	mov    %eax,%r8d
  20152b:	bf 13 00 00 00       	mov    $0x13,%edi
  201530:	e8 cf 00 00 00       	call   201604 <OsSystemCall>
  201535:	48 89 45 d0          	mov    %rax,-0x30(%rbp)
  201539:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  20153d:	48 8b 4d d8          	mov    -0x28(%rbp),%rcx
  201541:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  201545:	48 89 08             	mov    %rcx,(%rax)
  201548:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  20154c:	48 83 c4 30          	add    $0x30,%rsp
  201550:	5d                   	pop    %rbp
  201551:	c3                   	ret
  201552:	cc                   	int3
  201553:	cc                   	int3
  201554:	cc                   	int3
  201555:	cc                   	int3
  201556:	cc                   	int3
  201557:	cc                   	int3
  201558:	cc                   	int3
  201559:	cc                   	int3
  20155a:	cc                   	int3
  20155b:	cc                   	int3
  20155c:	cc                   	int3
  20155d:	cc                   	int3
  20155e:	cc                   	int3
  20155f:	cc                   	int3

0000000000201560 <OsFileSeek>:
  201560:	55                   	push   %rbp
  201561:	48 89 e5             	mov    %rsp,%rbp
  201564:	48 83 ec 30          	sub    $0x30,%rsp
  201568:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  20156c:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  201570:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  201574:	48 89 4d e0          	mov    %rcx,-0x20(%rbp)
  201578:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  20157c:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  201580:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  201584:	31 c0                	xor    %eax,%eax
  201586:	31 c0                	xor    %eax,%eax
  201588:	41 89 c0             	mov    %eax,%r8d
  20158b:	bf 14 00 00 00       	mov    $0x14,%edi
  201590:	e8 6f 00 00 00       	call   201604 <OsSystemCall>
  201595:	48 89 45 d0          	mov    %rax,-0x30(%rbp)
  201599:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  20159d:	48 8b 4d d8          	mov    -0x28(%rbp),%rcx
  2015a1:	48 8b 45 e0          	mov    -0x20(%rbp),%rax
  2015a5:	48 89 08             	mov    %rcx,(%rax)
  2015a8:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  2015ac:	48 83 c4 30          	add    $0x30,%rsp
  2015b0:	5d                   	pop    %rbp
  2015b1:	c3                   	ret
  2015b2:	cc                   	int3
  2015b3:	cc                   	int3
  2015b4:	cc                   	int3
  2015b5:	cc                   	int3
  2015b6:	cc                   	int3
  2015b7:	cc                   	int3
  2015b8:	cc                   	int3
  2015b9:	cc                   	int3
  2015ba:	cc                   	int3
  2015bb:	cc                   	int3
  2015bc:	cc                   	int3
  2015bd:	cc                   	int3
  2015be:	cc                   	int3
  2015bf:	cc                   	int3

00000000002015c0 <OsFileControl>:
  2015c0:	55                   	push   %rbp
  2015c1:	48 89 e5             	mov    %rsp,%rbp
  2015c4:	48 83 ec 30          	sub    $0x30,%rsp
  2015c8:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
  2015cc:	48 89 75 f0          	mov    %rsi,-0x10(%rbp)
  2015d0:	48 89 55 e8          	mov    %rdx,-0x18(%rbp)
  2015d4:	48 89 4d e0          	mov    %rcx,-0x20(%rbp)
  2015d8:	48 8b 75 f8          	mov    -0x8(%rbp),%rsi
  2015dc:	48 8b 55 f0          	mov    -0x10(%rbp),%rdx
  2015e0:	48 8b 4d e8          	mov    -0x18(%rbp),%rcx
  2015e4:	4c 8b 45 e0          	mov    -0x20(%rbp),%r8
  2015e8:	bf 16 00 00 00       	mov    $0x16,%edi
  2015ed:	e8 12 00 00 00       	call   201604 <OsSystemCall>
  2015f2:	48 89 45 d0          	mov    %rax,-0x30(%rbp)
  2015f6:	48 89 55 d8          	mov    %rdx,-0x28(%rbp)
  2015fa:	48 8b 45 d0          	mov    -0x30(%rbp),%rax
  2015fe:	48 83 c4 30          	add    $0x30,%rsp
  201602:	5d                   	pop    %rbp
  201603:	c3                   	ret

0000000000201604 <OsSystemCall>:
  201604:	48 89 f8             	mov    %rdi,%rax
  201607:	48 89 f7             	mov    %rsi,%rdi
  20160a:	48 89 d6             	mov    %rdx,%rsi
  20160d:	48 89 ca             	mov    %rcx,%rdx
  201610:	0f 05                	syscall
  201612:	c3                   	ret

Disassembly of section .debug_abbrev:

0000000000000000 <.debug_abbrev>:
   0:	01 11                	add    %edx,(%rcx)
   2:	01 25 25 13 05 03    	add    %esp,0x3051325(%rip)        # 305132d <OsSystemCall+0x2e4fd29>
   8:	25 72 17 10 17       	and    $0x17101772,%eax
   d:	1b 25 11 01 55 23    	sbb    0x23550111(%rip),%esp        # 23550124 <OsSystemCall+0x2334eb20>
  13:	73 17                	jae    2c <strlen-0x2011e4>
  15:	74 17                	je     2e <strlen-0x2011e2>
  17:	00 00                	add    %al,(%rax)
  19:	02 34 00             	add    (%rax,%rax,1),%dh
  1c:	49 13 3a             	adc    (%r10),%rdi
  1f:	0b 3b                	or     (%rbx),%edi
  21:	0b 02                	or     (%rdx),%eax
  23:	18 00                	sbb    %al,(%rax)
  25:	00 03                	add    %al,(%rbx)
  27:	01 01                	add    %eax,(%rcx)
  29:	49 13 00             	adc    (%r8),%rax
  2c:	00 04 21             	add    %al,(%rcx,%riz,1)
  2f:	00 49 13             	add    %cl,0x13(%rcx)
  32:	37                   	(bad)
  33:	0b 00                	or     (%rax),%eax
  35:	00 05 26 00 49 13    	add    %al,0x13490026(%rip)        # 13490061 <OsSystemCall+0x1328ea5d>
  3b:	00 00                	add    %al,(%rax)
  3d:	06                   	(bad)
  3e:	24 00                	and    $0x0,%al
  40:	03 25 3e 0b 0b 0b    	add    0xb0b0b3e(%rip),%esp        # b0b0b84 <OsSystemCall+0xaeaf580>
  46:	00 00                	add    %al,(%rax)
  48:	07                   	(bad)
  49:	24 00                	and    $0x0,%al
  4b:	03 25 0b 0b 3e 0b    	add    0xb3e0b0b(%rip),%esp        # b3e0b5c <OsSystemCall+0xb1df558>
  51:	00 00                	add    %al,(%rax)
  53:	08 04 01             	or     %al,(%rcx,%rax,1)
  56:	49 13 0b             	adc    (%r11),%rcx
  59:	0b 3a                	or     (%rdx),%edi
  5b:	0b 3b                	or     (%rbx),%edi
  5d:	0b 00                	or     (%rax),%eax
  5f:	00 09                	add    %cl,(%rcx)
  61:	28 00                	sub    %al,(%rax)
  63:	03 25 1c 0f 00 00    	add    0xf1c(%rip),%esp        # f85 <strlen-0x20028b>
  69:	0a 16                	or     (%rsi),%dl
  6b:	00 49 13             	add    %cl,0x13(%rcx)
  6e:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0bae <OsSystemCall+0xb1af5aa>
  74:	00 00                	add    %al,(%rax)
  76:	0b 0f                	or     (%rdi),%ecx
  78:	00 49 13             	add    %cl,0x13(%rcx)
  7b:	00 00                	add    %al,(%rax)
  7d:	0c 13                	or     $0x13,%al
  7f:	00 03                	add    %al,(%rbx)
  81:	25 3c 19 00 00       	and    $0x193c,%eax
  86:	0d 2e 01 11 1b       	or     $0x1b11012e,%eax
  8b:	12 06                	adc    (%rsi),%al
  8d:	40 18 03             	rex sbb %al,(%rbx)
  90:	25 3a 0b 3b 0b       	and    $0xb3b0b3a,%eax
  95:	49 13 3f             	adc    (%r15),%rdi
  98:	19 00                	sbb    %eax,(%rax)
  9a:	00 0e                	add    %cl,(%rsi)
  9c:	05 00 02 18 03       	add    $0x3180200,%eax
  a1:	25 3a 0b 3b 0b       	and    $0xb3b0b3a,%eax
  a6:	49 13 00             	adc    (%r8),%rax
  a9:	00 0f                	add    %cl,(%rdi)
  ab:	34 00                	xor    $0x0,%al
  ad:	02 18                	add    (%rax),%bl
  af:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0bef <OsSystemCall+0xb1af5eb>
  b5:	49 13 00             	adc    (%r8),%rax
  b8:	00 10                	add    %dl,(%rax)
  ba:	2e 01 11             	cs add %edx,(%rcx)
  bd:	1b 12                	sbb    (%rdx),%edx
  bf:	06                   	(bad)
  c0:	40 18 03             	rex sbb %al,(%rbx)
  c3:	25 3a 0b 3b 0b       	and    $0xb3b0b3a,%eax
  c8:	3f                   	(bad)
  c9:	19 87 01 19 00 00    	sbb    %eax,0x1901(%rdi)
  cf:	11 05 00 02 18 3a    	adc    %eax,0x3a180200(%rip)        # 3a1802d5 <OsSystemCall+0x39f7ecd1>
  d5:	0b 3b                	or     (%rbx),%edi
  d7:	0b 49 13             	or     0x13(%rcx),%ecx
  da:	00 00                	add    %al,(%rax)
  dc:	12 0b                	adc    (%rbx),%cl
  de:	01 11                	add    %edx,(%rcx)
  e0:	1b 12                	sbb    (%rdx),%edx
  e2:	06                   	(bad)
  e3:	00 00                	add    %al,(%rax)
  e5:	13 2e                	adc    (%rsi),%ebp
  e7:	01 11                	add    %edx,(%rcx)
  e9:	1b 12                	sbb    (%rdx),%edx
  eb:	06                   	(bad)
  ec:	40 18 6e 25          	sbb    %bpl,0x25(%rsi)
  f0:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0c30 <OsSystemCall+0xb1af62c>
  f6:	49 13 00             	adc    (%r8),%rax
  f9:	00 14 30             	add    %dl,(%rax,%rsi,1)
  fc:	00 49 13             	add    %cl,0x13(%rcx)
  ff:	03 25 1c 0f 00 00    	add    0xf1c(%rip),%esp        # 1021 <strlen-0x2001ef>
 105:	15 2e 01 11 1b       	adc    $0x1b11012e,%eax
 10a:	12 06                	adc    (%rsi),%al
 10c:	40 18 6e 25          	sbb    %bpl,0x25(%rsi)
 110:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0c50 <OsSystemCall+0xb1af64c>
 116:	00 00                	add    %al,(%rax)
 118:	16                   	(bad)
 119:	39 01                	cmp    %eax,(%rcx)
 11b:	03 25 00 00 17 08    	add    0x8170000(%rip),%esp        # 8170121 <OsSystemCall+0x7f6eb1d>
 121:	00 3a                	add    %bh,(%rdx)
 123:	0b 3b                	or     (%rbx),%edi
 125:	0b 18                	or     (%rax),%ebx
 127:	13 00                	adc    (%rax),%eax
 129:	00 18                	add    %bl,(%rax)
 12b:	13 00                	adc    (%rax),%eax
 12d:	3c 19                	cmp    $0x19,%al
 12f:	00 00                	add    %al,(%rax)
 131:	19 16                	sbb    %edx,(%rsi)
 133:	00 49 13             	add    %cl,0x13(%rcx)
 136:	03 25 3a 0b 3b 05    	add    0x53b0b3a(%rip),%esp        # 53b0c76 <OsSystemCall+0x51af672>
 13c:	00 00                	add    %al,(%rax)
 13e:	1a 21                	sbb    (%rcx),%ah
 140:	00 49 13             	add    %cl,0x13(%rcx)
 143:	37                   	(bad)
 144:	05 00 00 1b 10       	add    $0x101b0000,%eax
 149:	00 49 13             	add    %cl,0x13(%rcx)
 14c:	00 00                	add    %al,(%rax)
 14e:	00 01                	add    %al,(%rcx)
 150:	11 01                	adc    %eax,(%rcx)
 152:	25 25 13 05 03       	and    $0x3051325,%eax
 157:	25 72 17 10 17       	and    $0x17101772,%eax
 15c:	1b 25 11 01 55 23    	sbb    0x23550111(%rip),%esp        # 23550273 <OsSystemCall+0x2334ec6f>
 162:	73 17                	jae    17b <strlen-0x201095>
 164:	74 17                	je     17d <strlen-0x201093>
 166:	00 00                	add    %al,(%rax)
 168:	02 04 01             	add    (%rcx,%rax,1),%al
 16b:	49 13 0b             	adc    (%r11),%rcx
 16e:	0b 3a                	or     (%rdx),%edi
 170:	0b 3b                	or     (%rbx),%edi
 172:	0b 00                	or     (%rax),%eax
 174:	00 03                	add    %al,(%rbx)
 176:	28 00                	sub    %al,(%rax)
 178:	03 25 1c 0f 00 00    	add    0xf1c(%rip),%esp        # 109a <strlen-0x200176>
 17e:	04 24                	add    $0x24,%al
 180:	00 03                	add    %al,(%rbx)
 182:	25 3e 0b 0b 0b       	and    $0xb0b0b3e,%eax
 187:	00 00                	add    %al,(%rax)
 189:	05 16 00 49 13       	add    $0x13490016,%eax
 18e:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0cce <OsSystemCall+0xb1af6ca>
 194:	00 00                	add    %al,(%rax)
 196:	06                   	(bad)
 197:	0f 00 49 13          	str    0x13(%rcx)
 19b:	00 00                	add    %al,(%rax)
 19d:	07                   	(bad)
 19e:	13 00                	adc    (%rax),%eax
 1a0:	03 25 3c 19 00 00    	add    0x193c(%rip),%esp        # 1ae2 <strlen-0x1ff72e>
 1a6:	08 2e                	or     %ch,(%rsi)
 1a8:	01 11                	add    %edx,(%rcx)
 1aa:	1b 12                	sbb    (%rdx),%edx
 1ac:	06                   	(bad)
 1ad:	40 18 03             	rex sbb %al,(%rbx)
 1b0:	25 3a 0b 3b 0b       	and    $0xb3b0b3a,%eax
 1b5:	27                   	(bad)
 1b6:	19 49 13             	sbb    %ecx,0x13(%rcx)
 1b9:	3f                   	(bad)
 1ba:	19 00                	sbb    %eax,(%rax)
 1bc:	00 09                	add    %cl,(%rcx)
 1be:	05 00 02 18 03       	add    $0x3180200,%eax
 1c3:	25 3a 0b 3b 0b       	and    $0xb3b0b3a,%eax
 1c8:	49 13 00             	adc    (%r8),%rax
 1cb:	00 0a                	add    %cl,(%rdx)
 1cd:	34 00                	xor    $0x0,%al
 1cf:	02 18                	add    (%rax),%bl
 1d1:	03 25 3a 0b 3b 0b    	add    0xb3b0b3a(%rip),%esp        # b3b0d11 <OsSystemCall+0xb1af70d>
 1d7:	49 13 00             	adc    (%r8),%rax
 1da:	00 0b                	add    %cl,(%rbx)
 1dc:	26 00 49 13          	es add %cl,0x13(%rcx)
 1e0:	00 00                	add    %al,(%rax)
 1e2:	0c 13                	or     $0x13,%al
 1e4:	01 03                	add    %eax,(%rbx)
 1e6:	25 0b 0b 3a 0b       	and    $0xb3a0b0b,%eax
 1eb:	3b 0b                	cmp    (%rbx),%ecx
 1ed:	00 00                	add    %al,(%rax)
 1ef:	0d 0d 00 03 25       	or     $0x2503000d,%eax
 1f4:	49 13 3a             	adc    (%r10),%rdi
 1f7:	0b 3b                	or     (%rbx),%edi
 1f9:	0b 38                	or     (%rax),%edi
 1fb:	0b 00                	or     (%rax),%eax
 1fd:	00 0e                	add    %cl,(%rsi)
 1ff:	0f 00 00             	sldt   (%rax)
 202:	00 0f                	add    %cl,(%rdi)
 204:	26 00 00             	es add %al,(%rax)
 207:	00 00                	add    %al,(%rax)
 209:	01 11                	add    %edx,(%rcx)
 20b:	01 10                	add    %edx,(%rax)
 20d:	17                   	(bad)
 20e:	11 01                	adc    %eax,(%rcx)
 210:	12 01                	adc    (%rcx),%al
 212:	03 08                	add    (%rax),%ecx
 214:	1b 08                	sbb    (%rax),%ecx
 216:	25 08 13 05 00       	and    $0x51308,%eax
 21b:	00 02                	add    %al,(%rdx)
 21d:	0a 00                	or     (%rax),%al
 21f:	03 08                	add    (%rax),%ecx
 221:	3a 06                	cmp    (%rsi),%al
 223:	3b 06                	cmp    (%rsi),%eax
 225:	11 01                	adc    %eax,(%rcx)
 227:	00 00                	add    %al,(%rax)
	...

Disassembly of section .debug_info:

0000000000000000 <.debug_info>:
   0:	3f                   	(bad)
   1:	04 00                	add    $0x0,%al
   3:	00 05 00 01 08 00    	add    %al,0x80100(%rip)        # 80109 <strlen-0x181107>
   9:	00 00                	add    %al,(%rax)
   b:	00 01                	add    %al,(%rcx)
   d:	00 21                	add    %ah,(%rcx)
   f:	00 01                	add    %al,(%rcx)
  11:	08 00                	or     %al,(%rax)
  13:	00 00                	add    %al,(%rax)
  15:	00 00                	add    %al,(%rax)
  17:	00 00                	add    %al,(%rax)
  19:	02 00                	add    (%rax),%al
	...
  23:	08 00                	or     %al,(%rax)
  25:	00 00                	add    %al,(%rax)
  27:	0c 00                	or     $0x0,%al
  29:	00 00                	add    %al,(%rax)
  2b:	02 35 00 00 00 00    	add    0x0(%rip),%dh        # 31 <strlen-0x2011df>
  31:	26 02 a1 00 03 41 00 	es add 0x410300(%rcx),%ah
  38:	00 00                	add    %al,(%rax)
  3a:	04 4a                	add    $0x4a,%al
  3c:	00 00                	add    %al,(%rax)
  3e:	00 15 00 05 46 00    	add    %dl,0x460500(%rip)        # 460544 <OsSystemCall+0x25ef40>
  44:	00 00                	add    %al,(%rax)
  46:	06                   	(bad)
  47:	03 06                	add    (%rsi),%eax
  49:	01 07                	add    %eax,(%rdi)
  4b:	04 08                	add    $0x8,%al
  4d:	07                   	(bad)
  4e:	02 58 00             	add    0x0(%rax),%bl
  51:	00 00                	add    %al,(%rax)
  53:	00 29                	add    %ch,(%rcx)
  55:	02 a1 01 03 41 00    	add    0x410301(%rcx),%ah
  5b:	00 00                	add    %al,(%rax)
  5d:	04 4a                	add    $0x4a,%al
  5f:	00 00                	add    %al,(%rax)
  61:	00 2a                	add    %ch,(%rdx)
  63:	00 02                	add    %al,(%rdx)
  65:	58                   	pop    %rax
  66:	00 00                	add    %al,(%rax)
  68:	00 00                	add    %al,(%rax)
  6a:	33 02                	xor    (%rdx),%eax
  6c:	a1 02 02 78 00 00 00 	movabs 0x3800000000780202,%eax
  73:	00 38 
  75:	02 a1 03 03 41 00    	add    0x410303(%rcx),%ah
  7b:	00 00                	add    %al,(%rax)
  7d:	04 4a                	add    $0x4a,%al
  7f:	00 00                	add    %al,(%rax)
  81:	00 23                	add    %ah,(%rbx)
  83:	00 08                	add    %cl,(%rax)
  85:	d4                   	(bad)
  86:	00 00                	add    %al,(%rax)
  88:	00 04 01             	add    %al,(%rcx,%rax,1)
  8b:	1f                   	(bad)
  8c:	09 06                	or     %eax,(%rsi)
  8e:	10 09                	adc    %cl,(%rcx)
  90:	07                   	(bad)
  91:	11 09                	adc    %ecx,(%rcx)
  93:	08 12                	or     %dl,(%rdx)
  95:	09 09                	or     %ecx,(%rcx)
  97:	13 09                	adc    (%rcx),%ecx
  99:	0a 14 09             	or     (%rcx,%rcx,1),%dl
  9c:	0b 15 09 0c 16 09    	or     0x9160c09(%rip),%edx        # 9160cab <OsSystemCall+0x8f5f6a7>
  a2:	0d 20 09 0e 21       	or     $0x210e0920,%eax
  a7:	09 0f                	or     %ecx,(%rdi)
  a9:	30 09                	xor    %cl,(%rcx)
  ab:	10 31                	adc    %dh,(%rcx)
  ad:	09 11                	or     %edx,(%rcx)
  af:	32 09                	xor    (%rcx),%cl
  b1:	12 40 09             	adc    0x9(%rax),%al
  b4:	13 41 09             	adc    0x9(%rcx),%eax
  b7:	14 42                	adc    $0x42,%al
  b9:	09 15 43 09 16 50    	or     %edx,0x50160943(%rip)        # 50160a02 <OsSystemCall+0x4ff5f3fe>
  bf:	09 17                	or     %edx,(%rdi)
  c1:	51                   	push   %rcx
  c2:	09 18                	or     %ebx,(%rax)
  c4:	52                   	push   %rdx
  c5:	09 19                	or     %ebx,(%rcx)
  c7:	60                   	(bad)
  c8:	09 1a                	or     %ebx,(%rdx)
  ca:	61                   	(bad)
  cb:	09 1b                	or     %ebx,(%rbx)
  cd:	90                   	nop
  ce:	01 09                	add    %ecx,(%rcx)
  d0:	1c 80                	sbb    $0x80,%al
  d2:	02 00                	add    (%rax),%al
  d4:	06                   	(bad)
  d5:	05 07 04 08 d4       	add    $0xd4080407,%eax
  da:	00 00                	add    %al,(%rax)
  dc:	00 04 01             	add    %al,(%rcx,%rax,1)
  df:	73 09                	jae    ea <strlen-0x201126>
  e1:	1d 01 09 1e 02       	sbb    $0x21e0901,%eax
  e6:	09 1f                	or     %ebx,(%rdi)
  e8:	04 09                	add    $0x9,%al
  ea:	20 08                	and    %cl,(%rax)
  ec:	09 21                	or     %esp,(%rcx)
  ee:	80 02 09             	addb   $0x9,(%rdx)
  f1:	22 80 04 09 23 80    	and    -0x7fdcf6fc(%rax),%al
  f7:	06                   	(bad)
  f8:	00 0a                	add    %cl,(%rdx)
  fa:	01 01                	add    %eax,(%rcx)
  fc:	00 00                	add    %al,(%rax)
  fe:	25 01 4e 0b 06       	and    $0x60b4e01,%eax
 103:	01 00                	add    %eax,(%rax)
 105:	00 0c 24             	add    %cl,(%rsp)
 108:	0a 10                	or     (%rax),%dl
 10a:	01 00                	add    %eax,(%rax)
 10c:	00 27                	add    %ah,(%rdi)
 10e:	02 66 06             	add    0x6(%rsi),%ah
 111:	26 07                	es (bad)
 113:	08 0d 04 40 00 00    	or     %cl,0x4004(%rip)        # 411d <strlen-0x1fd0f3>
 119:	00 01                	add    %al,(%rcx)
 11b:	56                   	push   %rsi
 11c:	4b 00 06             	rex.WXB add %al,(%r14)
 11f:	16                   	(bad)
 120:	04 00                	add    $0x0,%al
 122:	00 0e                	add    %cl,(%rsi)
 124:	02 91 78 55 00 06    	add    0x6005578(%rcx),%dl
 12a:	26 04 00             	es add $0x0,%al
 12d:	00 0f                	add    %cl,(%rdi)
 12f:	02 91 70 56 00 07    	add    0x7005670(%rcx),%dl
 135:	16                   	(bad)
 136:	04 00                	add    $0x0,%al
 138:	00 00                	add    %al,(%rax)
 13a:	10 05 c2 00 00 00    	adc    %al,0xc2(%rip)        # 202 <strlen-0x20100e>
 140:	01 56 4d             	add    %edx,0x4d(%rsi)
 143:	00 24 11             	add    %ah,(%rcx,%rdx,1)
 146:	02 91 78 00 24 08    	add    0x8240078(%rcx),%dl
 14c:	01 00                	add    %eax,(%rax)
 14e:	00 11                	add    %dl,(%rcx)
 150:	02 91 70 00 24 08    	add    0x8240070(%rcx),%dl
 156:	01 00                	add    %eax,(%rax)
 158:	00 11                	add    %dl,(%rcx)
 15a:	02 91 68 00 24 08    	add    0x8240068(%rcx),%dl
 160:	01 00                	add    %eax,(%rax)
 162:	00 0f                	add    %cl,(%rdi)
 164:	02 91 60 57 00 25    	add    0x25005760(%rcx),%dl
 16a:	f9                   	stc
 16b:	00 00                	add    %al,(%rax)
 16d:	00 0f                	add    %cl,(%rdi)
 16f:	03 91 d0 7d 59 00    	add    0x597dd0(%rcx),%edx
 175:	2e 2b 04 00          	cs sub (%rax,%rax,1),%eax
 179:	00 0f                	add    %cl,(%rdi)
 17b:	03 91 c8 7d 5a 00    	add    0x5a7dc8(%rcx),%edx
 181:	2f                   	(bad)
 182:	16                   	(bad)
 183:	04 00                	add    $0x0,%al
 185:	00 12                	add    %dl,(%rdx)
 187:	06                   	(bad)
 188:	29 00                	sub    %eax,(%rax)
 18a:	00 00                	add    %al,(%rax)
 18c:	0f 02 91 58 58 00 26 	lar    0x26005858(%rcx),%edx
 193:	1e                   	(bad)
 194:	04 00                	add    $0x0,%al
 196:	00 00                	add    %al,(%rax)
 198:	12 07                	adc    (%rdi),%al
 19a:	44 00 00             	add    %r8b,(%rax)
 19d:	00 0f                	add    %cl,(%rdi)
 19f:	03 91 c0 7d 58 00    	add    0x587dc0(%rcx),%edx
 1a5:	30 1e                	xor    %bl,(%rsi)
 1a7:	04 00                	add    $0x0,%al
 1a9:	00 00                	add    %al,(%rax)
 1ab:	00 13                	add    %dl,(%rbx)
 1ad:	08 44 00 00          	or     %al,0x0(%rax,%rax,1)
 1b1:	00 01                	add    %al,(%rcx)
 1b3:	56                   	push   %rsi
 1b4:	4f 50                	rex.WRXB push %r8
 1b6:	00 1a                	add    %bl,(%rdx)
 1b8:	1e                   	(bad)
 1b9:	04 00                	add    $0x0,%al
 1bb:	00 0e                	add    %cl,(%rsi)
 1bd:	02 91 78 5b 00 1a    	add    0x1a005b78(%rcx),%dl
 1c3:	38 04 00             	cmp    %al,(%rax,%rax,1)
 1c6:	00 0e                	add    %cl,(%rsi)
 1c8:	02 91 70 5c 00 1a    	add    0x1a005c70(%rcx),%dl
 1ce:	3d 04 00 00 0f       	cmp    $0xf000004,%eax
 1d3:	02 91 68 5d 00 1b    	add    0x1b005d68(%rcx),%dl
 1d9:	26 04 00             	es add $0x0,%al
 1dc:	00 0f                	add    %cl,(%rdi)
 1de:	02 91 60 5e 00 1c    	add    0x1c005e60(%rcx),%dl
 1e4:	26 04 00             	es add $0x0,%al
 1e7:	00 14 10             	add    %dl,(%rax,%rdx,1)
 1ea:	01 00                	add    %eax,(%rax)
 1ec:	00 4e 15             	add    %cl,0x15(%rsi)
 1ef:	00 15 09 46 00 00    	add    %dl,0x4609(%rip)        # 47fe <strlen-0x1fca12>
 1f5:	00 01                	add    %al,(%rcx)
 1f7:	56                   	push   %rsi
 1f8:	52                   	push   %rdx
 1f9:	53                   	push   %rbx
 1fa:	00 12                	add    %dl,(%rdx)
 1fc:	0e                   	(bad)
 1fd:	02 91 78 5f 00 12    	add    0x12005f78(%rcx),%dl
 203:	26 04 00             	es add $0x0,%al
 206:	00 0f                	add    %cl,(%rdi)
 208:	02 91 70 5d 00 13    	add    0x13005d70(%rcx),%dl
 20e:	26 04 00             	es add $0x0,%al
 211:	00 0f                	add    %cl,(%rdi)
 213:	02 91 68 5e 00 14    	add    0x14005e68(%rcx),%dl
 219:	26 04 00             	es add $0x0,%al
 21c:	00 00                	add    %al,(%rax)
 21e:	15 0a 3a 00 00       	adc    $0x3a0a,%eax
 223:	00 01                	add    %al,(%rcx)
 225:	56                   	push   %rsi
 226:	54                   	push   %rsp
 227:	53                   	push   %rbx
 228:	00 0e                	add    %cl,(%rsi)
 22a:	0e                   	(bad)
 22b:	02 91 78 5d 00 0e    	add    0xe005d78(%rcx),%dl
 231:	26 04 00             	es add $0x0,%al
 234:	00 0e                	add    %cl,(%rsi)
 236:	02 91 70 5e 00 0e    	add    0xe005e70(%rcx),%dl
 23c:	26 04 00             	es add $0x0,%al
 23f:	00 00                	add    %al,(%rax)
 241:	16                   	(bad)
 242:	28 17                	sub    %dl,(%rdi)
 244:	04 3d                	add    $0x3d,%al
 246:	0f 03 00             	lsl    (%rax),%eax
 249:	00 17                	add    %dl,(%rdi)
 24b:	05 33 18 03 00       	add    $0x31833,%eax
 250:	00 17                	add    %dl,(%rdi)
 252:	05 34 25 03 00       	add    $0x32534,%eax
 257:	00 17                	add    %dl,(%rdi)
 259:	05 35 31 03 00       	add    $0x33135,%eax
 25e:	00 17                	add    %dl,(%rdi)
 260:	05 36 3d 03 00       	add    $0x33d36,%eax
 265:	00 17                	add    %dl,(%rdi)
 267:	05 38 49 03 00       	add    $0x34938,%eax
 26c:	00 17                	add    %dl,(%rdi)
 26e:	05 39 52 03 00       	add    $0x35239,%eax
 273:	00 17                	add    %dl,(%rdi)
 275:	05 3a 5b 03 00       	add    $0x35b3a,%eax
 27a:	00 17                	add    %dl,(%rdi)
 27c:	05 3b 63 03 00       	add    $0x3633b,%eax
 281:	00 17                	add    %dl,(%rdi)
 283:	05 3d 6b 03 00       	add    $0x36b3d,%eax
 288:	00 17                	add    %dl,(%rdi)
 28a:	05 3e 74 03 00       	add    $0x3743e,%eax
 28f:	00 17                	add    %dl,(%rdi)
 291:	05 3f 7d 03 00       	add    $0x37d3f,%eax
 296:	00 17                	add    %dl,(%rdi)
 298:	05 40 85 03 00       	add    $0x38540,%eax
 29d:	00 17                	add    %dl,(%rdi)
 29f:	05 42 8d 03 00       	add    $0x38d42,%eax
 2a4:	00 17                	add    %dl,(%rdi)
 2a6:	05 43 96 03 00       	add    $0x39643,%eax
 2ab:	00 17                	add    %dl,(%rdi)
 2ad:	05 45 9f 03 00       	add    $0x39f45,%eax
 2b2:	00 17                	add    %dl,(%rdi)
 2b4:	05 46 ac 03 00       	add    $0x3ac46,%eax
 2b9:	00 17                	add    %dl,(%rdi)
 2bb:	05 47 b8 03 00       	add    $0x3b847,%eax
 2c0:	00 17                	add    %dl,(%rdi)
 2c2:	05 48 08 01 00       	add    $0x10848,%eax
 2c7:	00 17                	add    %dl,(%rdi)
 2c9:	05 4a c0 03 00       	add    $0x3c04a,%eax
 2ce:	00 17                	add    %dl,(%rdi)
 2d0:	05 4b c9 03 00       	add    $0x3c94b,%eax
 2d5:	00 17                	add    %dl,(%rdi)
 2d7:	05 4c d2 03 00       	add    $0x3d24c,%eax
 2dc:	00 17                	add    %dl,(%rdi)
 2de:	05 4d da 03 00       	add    $0x3da4d,%eax
 2e3:	00 17                	add    %dl,(%rdi)
 2e5:	05 4f e2 03 00       	add    $0x3e24f,%eax
 2ea:	00 17                	add    %dl,(%rdi)
 2ec:	05 50 eb 03 00       	add    $0x3eb50,%eax
 2f1:	00 17                	add    %dl,(%rdi)
 2f3:	05 51 f4 03 00       	add    $0x3f451,%eax
 2f8:	00 17                	add    %dl,(%rdi)
 2fa:	05 52 fc 03 00       	add    $0x3fc52,%eax
 2ff:	00 17                	add    %dl,(%rdi)
 301:	05 54 04 04 00       	add    $0x40454,%eax
 306:	00 17                	add    %dl,(%rdi)
 308:	05 55 0d 04 00       	add    $0x40d55,%eax
 30d:	00 00                	add    %al,(%rax)
 30f:	0a 17                	or     (%rdi),%dl
 311:	03 00                	add    (%rax),%eax
 313:	00 29                	add    %ch,(%rcx)
 315:	03 18                	add    (%rax),%ebx
 317:	18 19                	sbb    %bl,(%rcx)
 319:	21 03                	and    %eax,(%rbx)
 31b:	00 00                	add    %al,(%rax)
 31d:	2b 02                	sub    (%rdx),%eax
 31f:	0c 01                	or     $0x1,%al
 321:	06                   	(bad)
 322:	2a 06                	sub    (%rsi),%al
 324:	01 0a                	add    %ecx,(%rdx)
 326:	2d 03 00 00 2d       	sub    $0x2d000003,%eax
 32b:	02 f5                	add    %ch,%dh
 32d:	06                   	(bad)
 32e:	2c 05                	sub    $0x5,%al
 330:	02 0a                	add    (%rdx),%cl
 332:	39 03                	cmp    %eax,(%rbx)
 334:	00 00                	add    %al,(%rax)
 336:	2f                   	(bad)
 337:	02 c5                	add    %ch,%al
 339:	06                   	(bad)
 33a:	2e 05 04 0a 45 03    	cs add $0x3450a04,%eax
 340:	00 00                	add    %al,(%rax)
 342:	31 02                	xor    %eax,(%rdx)
 344:	64 06                	fs (bad)
 346:	30 05 08 19 18 03    	xor    %al,0x3181908(%rip)        # 3181c54 <OsSystemCall+0x2f80650>
 34c:	00 00                	add    %al,(%rax)
 34e:	32 02                	xor    (%rdx),%al
 350:	18 01                	sbb    %al,(%rcx)
 352:	19 25 03 00 00 33    	sbb    %esp,0x33000003(%rip)        # 3300035b <OsSystemCall+0x32dfed57>
 358:	02 05 01 0a 31 03    	add    0x3310a01(%rip),%al        # 3310d5f <OsSystemCall+0x310f75b>
 35e:	00 00                	add    %al,(%rax)
 360:	34 02                	xor    $0x2,%al
 362:	de 0a                	fimuls (%rdx)
 364:	3d 03 00 00 35       	cmp    $0x35000003,%eax
 369:	02 7c 19 18          	add    0x18(%rcx,%rbx,1),%bh
 36d:	03 00                	add    (%rax),%eax
 36f:	00 36                	add    %dh,(%rsi)
 371:	02 16                	add    (%rsi),%dl
 373:	01 19                	add    %ebx,(%rcx)
 375:	25 03 00 00 37       	and    $0x37000003,%eax
 37a:	02 03                	add    (%rbx),%al
 37c:	01 0a                	add    %ecx,(%rdx)
 37e:	31 03                	xor    %eax,(%rbx)
 380:	00 00                	add    %al,(%rax)
 382:	38 02                	cmp    %al,(%rdx)
 384:	dc 0a                	fmull  (%rdx)
 386:	3d 03 00 00 39       	cmp    $0x39000003,%eax
 38b:	02 7a 19             	add    0x19(%rdx),%bh
 38e:	45 03 00             	add    (%r8),%r8d
 391:	00 3a                	add    %bh,(%rdx)
 393:	02 34 01             	add    (%rcx,%rax,1),%dh
 396:	19 45 03             	sbb    %eax,0x3(%rbp)
 399:	00 00                	add    %al,(%rax)
 39b:	3b 02                	cmp    (%rdx),%eax
 39d:	27                   	(bad)
 39e:	01 19                	add    %ebx,(%rcx)
 3a0:	a8 03                	test   $0x3,%al
 3a2:	00 00                	add    %al,(%rax)
 3a4:	3d 02 0e 01 06       	cmp    $0x6010e02,%eax
 3a9:	3c 08                	cmp    $0x8,%al
 3ab:	01 0a                	add    %ecx,(%rdx)
 3ad:	b4 03                	mov    $0x3,%ah
 3af:	00 00                	add    %al,(%rax)
 3b1:	3f                   	(bad)
 3b2:	02 f7                	add    %bh,%dh
 3b4:	06                   	(bad)
 3b5:	3e 07                	ds (bad)
 3b7:	02 0a                	add    (%rdx),%cl
 3b9:	d4                   	(bad)
 3ba:	00 00                	add    %al,(%rax)
 3bc:	00 40 02             	add    %al,0x2(%rax)
 3bf:	ca 19 9f             	lret   $0x9f19
 3c2:	03 00                	add    (%rax),%eax
 3c4:	00 41 02             	add    %al,0x2(%rcx)
 3c7:	19 01                	sbb    %eax,(%rcx)
 3c9:	19 ac 03 00 00 42 02 	sbb    %ebp,0x2420000(%rbx,%rax,1)
 3d0:	06                   	(bad)
 3d1:	01 0a                	add    %ecx,(%rdx)
 3d3:	b8 03 00 00 43       	mov    $0x43000003,%eax
 3d8:	02 df                	add    %bh,%bl
 3da:	0a 08                	or     (%rax),%cl
 3dc:	01 00                	add    %eax,(%rax)
 3de:	00 44 02 7d          	add    %al,0x7d(%rdx,%rax,1)
 3e2:	19 9f 03 00 00 45    	sbb    %ebx,0x45000003(%rdi)
 3e8:	02 17                	add    (%rdi),%dl
 3ea:	01 19                	add    %ebx,(%rcx)
 3ec:	ac                   	lods   %ds:(%rsi),%al
 3ed:	03 00                	add    (%rax),%eax
 3ef:	00 46 02             	add    %al,0x2(%rsi)
 3f2:	04 01                	add    $0x1,%al
 3f4:	0a b8 03 00 00 47    	or     0x47000003(%rax),%bh
 3fa:	02 dd                	add    %ch,%bl
 3fc:	0a 08                	or     (%rax),%cl
 3fe:	01 00                	add    %eax,(%rax)
 400:	00 48 02             	add    %cl,0x2(%rax)
 403:	7b 19                	jnp    41e <strlen-0x200df2>
 405:	10 01                	adc    %al,(%rcx)
 407:	00 00                	add    %al,(%rax)
 409:	49 02 35 01 19 10 01 	rex.WB add 0x1101901(%rip),%sil        # 1101d11 <OsSystemCall+0xf0070d>
 410:	00 00                	add    %al,(%rax)
 412:	4a 02 2e             	rex.WX add (%rsi),%bpl
 415:	01 0a                	add    %ecx,(%rdx)
 417:	10 01                	adc    %al,(%rcx)
 419:	00 00                	add    %al,(%rax)
 41b:	4c 06                	rex.WR (bad)
 41d:	12 0a                	adc    (%rdx),%cl
 41f:	08 01                	or     %al,(%rcx)
 421:	00 00                	add    %al,(%rax)
 423:	51                   	push   %rcx
 424:	07                   	(bad)
 425:	0a 0b                	or     (%rbx),%cl
 427:	41 00 00             	add    %al,(%r8)
 42a:	00 03                	add    %al,(%rbx)
 42c:	46 00 00             	rex.RX add %r8b,(%rax)
 42f:	00 1a                	add    %bl,(%rdx)
 431:	4a 00 00             	rex.WX add %al,(%rax)
 434:	00 00                	add    %al,(%rax)
 436:	01 00                	add    %eax,(%rax)
 438:	1b 35 00 00 00 0b    	sbb    0xb000000(%rip),%esi        # b00043e <OsSystemCall+0xadfee3a>
 43e:	f9                   	stc
 43f:	00 00                	add    %al,(%rax)
 441:	00 00                	add    %al,(%rax)
 443:	83 02 00             	addl   $0x0,(%rdx)
 446:	00 05 00 01 08 4f    	add    %al,0x4f080100(%rip)        # 4f08054c <OsSystemCall+0x4ee7ef48>
 44c:	01 00                	add    %eax,(%rax)
 44e:	00 01                	add    %al,(%rcx)
 450:	00 1d 00 01 90 01    	add    %bl,0x1900100(%rip)        # 1900556 <OsSystemCall+0x16fef52>
 456:	00 00                	add    %al,(%rax)
 458:	85 01                	test   %eax,(%rcx)
 45a:	00 00                	add    %al,(%rax)
 45c:	02 00                	add    (%rax),%al
	...
 466:	68 00 00 00 2d       	push   $0x2d000000
 46b:	00 00                	add    %al,(%rax)
 46d:	00 02                	add    %al,(%rdx)
 46f:	7b 00                	jnp    471 <strlen-0x200d9f>
 471:	00 00                	add    %al,(%rax)
 473:	04 01                	add    $0x1,%al
 475:	1f                   	(bad)
 476:	03 04 10             	add    (%rax,%rdx,1),%eax
 479:	03 05 11 03 06 12    	add    0x12060311(%rip),%eax        # 12060790 <OsSystemCall+0x11e5f18c>
 47f:	03 07                	add    (%rdi),%eax
 481:	13 03                	adc    (%rbx),%eax
 483:	08 14 03             	or     %dl,(%rbx,%rax,1)
 486:	09 15 03 0a 16 03    	or     %edx,0x3160a03(%rip)        # 3160e8f <OsSystemCall+0x2f5f88b>
 48c:	0b 20                	or     (%rax),%esp
 48e:	03 0c 21             	add    (%rcx,%riz,1),%ecx
 491:	03 0d 30 03 0e 31    	add    0x310e0330(%rip),%ecx        # 310e07c7 <OsSystemCall+0x30edf1c3>
 497:	03 0f                	add    (%rdi),%ecx
 499:	32 03                	xor    (%rbx),%al
 49b:	10 40 03             	adc    %al,0x3(%rax)
 49e:	11 41 03             	adc    %eax,0x3(%rcx)
 4a1:	12 42 03             	adc    0x3(%rdx),%al
 4a4:	13 43 03             	adc    0x3(%rbx),%eax
 4a7:	14 50                	adc    $0x50,%al
 4a9:	03 15 51 03 16 52    	add    0x52160351(%rip),%edx        # 52160800 <OsSystemCall+0x51f5f1fc>
 4af:	03 17                	add    (%rdi),%edx
 4b1:	60                   	(bad)
 4b2:	03 18                	add    (%rax),%ebx
 4b4:	61                   	(bad)
 4b5:	03 19                	add    (%rcx),%ebx
 4b7:	90                   	nop
 4b8:	01 03                	add    %eax,(%rbx)
 4ba:	1a 80 02 00 04 03    	sbb    0x3040002(%rax),%al
 4c0:	07                   	(bad)
 4c1:	04 05                	add    $0x5,%al
 4c3:	87 00                	xchg   %eax,(%rax)
 4c5:	00 00                	add    %al,(%rax)
 4c7:	1c 02                	sbb    $0x2,%al
 4c9:	66 04 1b             	data16 add $0x1b,%al
 4cc:	07                   	(bad)
 4cd:	08 05 93 00 00 00    	or     %al,0x93(%rip)        # 566 <strlen-0x200caa>
 4d3:	1e                   	(bad)
 4d4:	01 4e 06             	add    %ecx,0x6(%rsi)
 4d7:	98                   	cwtl
 4d8:	00 00                	add    %al,(%rax)
 4da:	00 07                	add    %al,(%rdi)
 4dc:	1d 08 00 52 00       	sbb    $0x520008,%eax
 4e1:	00 00                	add    %al,(%rax)
 4e3:	01 56 1f             	add    %edx,0x1f(%rsi)
 4e6:	00 03                	add    %al,(%rbx)
 4e8:	23 02                	and    (%rdx),%eax
 4ea:	00 00                	add    %al,(%rax)
 4ec:	09 02                	or     %eax,(%rdx)
 4ee:	91                   	xchg   %eax,%ecx
 4ef:	78 26                	js     517 <strlen-0x200cf9>
 4f1:	00 03                	add    %al,(%rbx)
 4f3:	2b 02                	sub    (%rdx),%eax
 4f5:	00 00                	add    %al,(%rax)
 4f7:	09 02                	or     %eax,(%rdx)
 4f9:	91                   	xchg   %eax,%ecx
 4fa:	70 28                	jo     524 <strlen-0x200cec>
 4fc:	00 03                	add    %al,(%rbx)
 4fe:	2b 02                	sub    (%rdx),%eax
 500:	00 00                	add    %al,(%rax)
 502:	09 02                	or     %eax,(%rdx)
 504:	91                   	xchg   %eax,%ecx
 505:	68 29 00 03 39       	push   $0x39030029
 50a:	02 00                	add    (%rax),%al
 50c:	00 09                	add    %cl,(%rcx)
 50e:	02 91 60 2b 00 03    	add    0x3002b60(%rcx),%dl
 514:	41 02 00             	add    (%r8),%al
 517:	00 0a                	add    %cl,(%rdx)
 519:	02 91 50 2c 00 04    	add    0x4002c50(%rcx),%dl
 51f:	46 02 00             	rex.RX add (%rax),%r8b
 522:	00 00                	add    %al,(%rax)
 524:	08 01                	or     %al,(%rcx)
 526:	39 00                	cmp    %eax,(%rax)
 528:	00 00                	add    %al,(%rax)
 52a:	01 56 21             	add    %edx,0x21(%rsi)
 52d:	00 09                	add    %cl,(%rcx)
 52f:	23 02                	and    (%rdx),%eax
 531:	00 00                	add    %al,(%rax)
 533:	09 02                	or     %eax,(%rdx)
 535:	91                   	xchg   %eax,%ecx
 536:	78 30                	js     568 <strlen-0x200ca8>
 538:	00 09                	add    %cl,(%rcx)
 53a:	8b 00                	mov    (%rax),%eax
 53c:	00 00                	add    %al,(%rax)
 53e:	0a 02                	or     (%rdx),%al
 540:	91                   	xchg   %eax,%ecx
 541:	68 2c 00 0a 46       	push   $0x460a002c
 546:	02 00                	add    (%rax),%al
 548:	00 00                	add    %al,(%rax)
 54a:	08 02                	or     %al,(%rdx)
 54c:	52                   	push   %rdx
 54d:	00 00                	add    %al,(%rax)
 54f:	00 01                	add    %al,(%rcx)
 551:	56                   	push   %rsi
 552:	22 00                	and    (%rax),%al
 554:	0e                   	(bad)
 555:	23 02                	and    (%rdx),%eax
 557:	00 00                	add    %al,(%rax)
 559:	09 02                	or     %eax,(%rdx)
 55b:	91                   	xchg   %eax,%ecx
 55c:	78 30                	js     58e <strlen-0x200c82>
 55e:	00 0e                	add    %cl,(%rsi)
 560:	8b 00                	mov    (%rax),%eax
 562:	00 00                	add    %al,(%rax)
 564:	09 02                	or     %eax,(%rdx)
 566:	91                   	xchg   %eax,%ecx
 567:	70 31                	jo     59a <strlen-0x200c76>
 569:	00 0e                	add    %cl,(%rsi)
 56b:	5e                   	pop    %rsi
 56c:	02 00                	add    (%rax),%al
 56e:	00 09                	add    %cl,(%rcx)
 570:	02 91 68 32 00 0e    	add    0xe003268(%rcx),%dl
 576:	5e                   	pop    %rsi
 577:	02 00                	add    (%rax),%al
 579:	00 09                	add    %cl,(%rcx)
 57b:	02 91 60 33 00 0e    	add    0xe003360(%rcx),%dl
 581:	5f                   	pop    %rdi
 582:	02 00                	add    (%rax),%al
 584:	00 0a                	add    %cl,(%rdx)
 586:	02 91 50 2c 00 0f    	add    0xf002c50(%rcx),%dl
 58c:	46 02 00             	rex.RX add (%rax),%r8b
 58f:	00 00                	add    %al,(%rax)
 591:	08 03                	or     %al,(%rbx)
 593:	52                   	push   %rdx
 594:	00 00                	add    %al,(%rax)
 596:	00 01                	add    %al,(%rcx)
 598:	56                   	push   %rsi
 599:	23 00                	and    (%rax),%eax
 59b:	14 23                	adc    $0x23,%al
 59d:	02 00                	add    (%rax),%al
 59f:	00 09                	add    %cl,(%rcx)
 5a1:	02 91 78 30 00 14    	add    0x14003078(%rcx),%dl
 5a7:	8b 00                	mov    (%rax),%eax
 5a9:	00 00                	add    %al,(%rax)
 5ab:	09 02                	or     %eax,(%rdx)
 5ad:	91                   	xchg   %eax,%ecx
 5ae:	70 31                	jo     5e1 <strlen-0x200c2f>
 5b0:	00 14 64             	add    %dl,(%rsp,%riz,2)
 5b3:	02 00                	add    (%rax),%al
 5b5:	00 09                	add    %cl,(%rcx)
 5b7:	02 91 68 32 00 14    	add    0x14003268(%rcx),%dl
 5bd:	64 02 00             	add    %fs:(%rax),%al
 5c0:	00 09                	add    %cl,(%rcx)
 5c2:	02 91 60 34 00 14    	add    0x14003460(%rcx),%dl
 5c8:	5f                   	pop    %rdi
 5c9:	02 00                	add    (%rax),%al
 5cb:	00 0a                	add    %cl,(%rdx)
 5cd:	02 91 50 2c 00 15    	add    0x15002c50(%rcx),%dl
 5d3:	46 02 00             	rex.RX add (%rax),%r8b
 5d6:	00 00                	add    %al,(%rax)
 5d8:	08 04 52             	or     %al,(%rdx,%rdx,2)
 5db:	00 00                	add    %al,(%rax)
 5dd:	00 01                	add    %al,(%rcx)
 5df:	56                   	push   %rsi
 5e0:	24 00                	and    $0x0,%al
 5e2:	1a 23                	sbb    (%rbx),%ah
 5e4:	02 00                	add    (%rax),%al
 5e6:	00 09                	add    %cl,(%rcx)
 5e8:	02 91 78 30 00 1a    	add    0x1a003078(%rcx),%dl
 5ee:	8b 00                	mov    (%rax),%eax
 5f0:	00 00                	add    %al,(%rax)
 5f2:	09 02                	or     %eax,(%rdx)
 5f4:	91                   	xchg   %eax,%ecx
 5f5:	70 29                	jo     620 <strlen-0x200bf0>
 5f7:	00 1a                	add    %bl,(%rdx)
 5f9:	6a 02                	push   $0x2
 5fb:	00 00                	add    %al,(%rax)
 5fd:	09 02                	or     %eax,(%rdx)
 5ff:	91                   	xchg   %eax,%ecx
 600:	68 36 00 1a 72       	push   $0x721a0036
 605:	02 00                	add    (%rax),%al
 607:	00 09                	add    %cl,(%rcx)
 609:	02 91 60 39 00 1a    	add    0x1a003960(%rcx),%dl
 60f:	5f                   	pop    %rdi
 610:	02 00                	add    (%rax),%al
 612:	00 0a                	add    %cl,(%rdx)
 614:	02 91 50 2c 00 1b    	add    0x1b002c50(%rcx),%dl
 61a:	46 02 00             	rex.RX add (%rax),%r8b
 61d:	00 00                	add    %al,(%rax)
 61f:	08 05 44 00 00 00    	or     %al,0x44(%rip)        # 669 <strlen-0x200ba7>
 625:	01 56 25             	add    %edx,0x25(%rsi)
 628:	00 20                	add    %ah,(%rax)
 62a:	23 02                	and    (%rdx),%eax
 62c:	00 00                	add    %al,(%rax)
 62e:	09 02                	or     %eax,(%rdx)
 630:	91                   	xchg   %eax,%ecx
 631:	78 30                	js     663 <strlen-0x200bad>
 633:	00 20                	add    %ah,(%rax)
 635:	8b 00                	mov    (%rax),%eax
 637:	00 00                	add    %al,(%rax)
 639:	09 02                	or     %eax,(%rdx)
 63b:	91                   	xchg   %eax,%ecx
 63c:	70 3a                	jo     678 <strlen-0x200b98>
 63e:	00 20                	add    %ah,(%rax)
 640:	7f 00                	jg     642 <strlen-0x200bce>
 642:	00 00                	add    %al,(%rax)
 644:	09 02                	or     %eax,(%rdx)
 646:	91                   	xchg   %eax,%ecx
 647:	68 3b 00 20 5e       	push   $0x5e20003b
 64c:	02 00                	add    (%rax),%al
 64e:	00 09                	add    %cl,(%rcx)
 650:	02 91 60 3c 00 20    	add    0x20003c60(%rcx),%dl
 656:	7e 02                	jle    65a <strlen-0x200bb6>
 658:	00 00                	add    %al,(%rax)
 65a:	0a 02                	or     (%rdx),%al
 65c:	91                   	xchg   %eax,%ecx
 65d:	50                   	push   %rax
 65e:	2c 00                	sub    $0x0,%al
 660:	21 46 02             	and    %eax,0x2(%rsi)
 663:	00 00                	add    %al,(%rax)
 665:	00 05 7f 00 00 00    	add    %al,0x7f(%rip)        # 6ea <strlen-0x200b26>
 66b:	20 03                	and    %al,(%rbx)
 66d:	0a 06                	or     (%rsi),%al
 66f:	30 02                	xor    %al,(%rdx)
 671:	00 00                	add    %al,(%rax)
 673:	0b 35 02 00 00 04    	or     0x4000002(%rip),%esi        # 400067b <OsSystemCall+0x3dff077>
 679:	27                   	(bad)
 67a:	06                   	(bad)
 67b:	01 05 7f 00 00 00    	add    %eax,0x7f(%rip)        # 700 <strlen-0x200b10>
 681:	2a 01                	sub    (%rcx),%al
 683:	71 06                	jno    68b <strlen-0x200b85>
 685:	8b 00                	mov    (%rax),%eax
 687:	00 00                	add    %al,(%rax)
 689:	0c 2f                	or     $0x2f,%al
 68b:	10 01                	adc    %al,(%rcx)
 68d:	40 0d 2d 23 02 00    	rex or $0x2232d,%eax
 693:	00 01                	add    %al,(%rcx)
 695:	41 00 0d 2e 7f 00 00 	add    %cl,0x7f2e(%rip)        # 85ca <strlen-0x1f8c46>
 69c:	00 01                	add    %al,(%rcx)
 69e:	42 08 00             	rex.X or %al,(%rax)
 6a1:	0e                   	(bad)
 6a2:	06                   	(bad)
 6a3:	7f 00                	jg     6a5 <strlen-0x200b6b>
 6a5:	00 00                	add    %al,(%rax)
 6a7:	06                   	(bad)
 6a8:	69 02 00 00 0f 05    	imul   $0x50f0000,(%rdx),%eax
 6ae:	7f 00                	jg     6b0 <strlen-0x200b60>
 6b0:	00 00                	add    %al,(%rax)
 6b2:	35 01 6a 05 7a       	xor    $0x7a056a01,%eax
 6b7:	02 00                	add    (%rax),%al
 6b9:	00 38                	add    %bh,(%rax)
 6bb:	02 64 04 37          	add    0x37(%rsp,%rax,1),%ah
 6bf:	05 08 05 87 00       	add    $0x870508,%eax
 6c4:	00 00                	add    %al,(%rax)
 6c6:	3d 04 12 00 df       	cmp    $0xdf001204,%eax
 6cb:	00 00                	add    %al,(%rax)
 6cd:	00 05 00 01 08 09    	add    %al,0x9080100(%rip)        # 90807d3 <OsSystemCall+0x8e7f1cf>
 6d3:	02 00                	add    (%rax),%al
 6d5:	00 01                	add    %al,(%rcx)
 6d7:	5c                   	pop    %rsp
 6d8:	03 00                	add    (%rax),%eax
 6da:	00 04 16             	add    %al,(%rsi,%rdx,1)
 6dd:	20 00                	and    %al,(%rax)
 6df:	00 00                	add    %al,(%rax)
 6e1:	00 00                	add    %al,(%rax)
 6e3:	13 16                	adc    (%rsi),%edx
 6e5:	20 00                	and    %al,(%rax)
 6e7:	00 00                	add    %al,(%rax)
 6e9:	00 00                	add    %al,(%rax)
 6eb:	2e 2e 2f             	cs cs (bad)
 6ee:	73 75                	jae    765 <strlen-0x200aab>
 6f0:	62                   	(bad)
 6f1:	70 72                	jo     765 <strlen-0x200aab>
 6f3:	6f                   	outsl  %ds:(%rsi),(%dx)
 6f4:	6a 65                	push   $0x65
 6f6:	63 74 73 2f          	movsxd 0x2f(%rbx,%rsi,2),%esi
 6fa:	73 68                	jae    764 <strlen-0x200aac>
 6fc:	61                   	(bad)
 6fd:	72 65                	jb     764 <strlen-0x200aac>
 6ff:	64 2f                	fs (bad)
 701:	73 72                	jae    775 <strlen-0x200a9b>
 703:	63 2f                	movsxd (%rdi),%ebp
 705:	73 79                	jae    780 <strlen-0x200a90>
 707:	73 63                	jae    76c <strlen-0x200aa4>
 709:	61                   	(bad)
 70a:	6c                   	insb   (%dx),%es:(%rdi)
 70b:	6c                   	insb   (%dx),%es:(%rdi)
 70c:	2e 53                	cs push %rbx
 70e:	00 2f                	add    %ch,(%rdi)
 710:	68 6f 6d 65 2f       	push   $0x2f656d6f
 715:	65 6c                	gs insb (%dx),%es:(%rdi)
 717:	6c                   	insb   (%dx),%es:(%rdi)
 718:	69 6f 74 68 62 2f 67 	imul   $0x672f6268,0x74(%rdi),%ebp
 71f:	69 74 2f 62 65 7a 6f 	imul   $0x736f7a65,0x62(%rdi,%rbp,1),%esi
 726:	73 
 727:	2f                   	(bad)
 728:	73 79                	jae    7a3 <strlen-0x200a6d>
 72a:	73 74                	jae    7a0 <strlen-0x200a70>
 72c:	65 6d                	gs insl (%dx),%es:(%rdi)
 72e:	2f                   	(bad)
 72f:	62 75 69 6c 64       	(bad)
 734:	00 44 65 62          	add    %al,0x62(%rbp,%riz,2)
 738:	69 61 6e 20 63 6c 61 	imul   $0x616c6320,0x6e(%rcx),%esp
 73f:	6e                   	outsb  %ds:(%rsi),(%dx)
 740:	67 20 76 65          	and    %dh,0x65(%esi)
 744:	72 73                	jb     7b9 <strlen-0x200a57>
 746:	69 6f 6e 20 32 30 2e 	imul   $0x2e303220,0x6e(%rdi),%ebp
 74d:	30 2e                	xor    %ch,(%rsi)
 74f:	30 20                	xor    %ah,(%rax)
 751:	28 2b                	sub    %ch,(%rbx)
 753:	2b 32                	sub    (%rdx),%esi
 755:	30 32                	xor    %dh,(%rdx)
 757:	35 30 31 32 38       	xor    $0x38323130,%eax
 75c:	31 30                	xor    %esi,(%rax)
 75e:	35 33 30 39 2b       	xor    $0x2b393033,%eax
 763:	35 34 38 65 63       	xor    $0x63653834,%eax
 768:	64 65 34 32          	fs gs xor $0x32,%al
 76c:	38 38                	cmp    %bh,(%rax)
 76e:	36 2d 31 7e 65 78    	ss sub $0x78657e31,%eax
 774:	70 31                	jo     7a7 <strlen-0x200a69>
 776:	7e 32                	jle    7aa <strlen-0x200a66>
 778:	30 32                	xor    %dh,(%rdx)
 77a:	35 30 31 32 38       	xor    $0x38323130,%eax
 77f:	32 32                	xor    (%rdx),%dh
 781:	35 34 33 31 2e       	xor    $0x2e313334,%eax
 786:	31 32                	xor    %esi,(%rdx)
 788:	33 33                	xor    (%rbx),%esi
 78a:	29 00                	sub    %eax,(%rax)
 78c:	01 80 02 4f 73 53    	add    %eax,0x53734f02(%rax)
 792:	79 73                	jns    807 <strlen-0x200a09>
 794:	74 65                	je     7fb <strlen-0x200a15>
 796:	6d                   	insl   (%dx),%es:(%rdi)
 797:	43 61                	rex.XB (bad)
 799:	6c                   	insb   (%dx),%es:(%rdi)
 79a:	6c                   	insb   (%dx),%es:(%rdi)
 79b:	00 00                	add    %al,(%rax)
 79d:	00 00                	add    %al,(%rax)
 79f:	00 0a                	add    %cl,(%rdx)
 7a1:	00 00                	add    %al,(%rax)
 7a3:	00 04 16             	add    %al,(%rsi,%rdx,1)
 7a6:	20 00                	and    %al,(%rax)
 7a8:	00 00                	add    %al,(%rax)
 7aa:	00 00                	add    %al,(%rax)
	...

Disassembly of section .debug_rnglists:

0000000000000000 <.debug_rnglists>:
   0:	1d 00 00 00 05       	sbb    $0x5000000,%eax
   5:	00 08                	add    %cl,(%rax)
   7:	00 01                	add    %al,(%rcx)
   9:	00 00                	add    %al,(%rax)
   b:	00 04 00             	add    %al,(%rax,%rax,1)
   e:	00 00                	add    %al,(%rax)
  10:	03 04 40             	add    (%rax,%rax,2),%eax
  13:	03 05 c2 01 03 08    	add    0x80301c2(%rip),%eax        # 80301db <OsSystemCall+0x7e2ebd7>
  19:	44 03 09             	add    (%rcx),%r9d
  1c:	46 03 0a             	rex.RX add (%rdx),%r9d
  1f:	3a 00                	cmp    (%rax),%al
  21:	1f                   	(bad)
  22:	00 00                	add    %al,(%rax)
  24:	00 05 00 08 00 01    	add    %al,0x1000800(%rip)        # 100082a <OsSystemCall+0xdff226>
  2a:	00 00                	add    %al,(%rax)
  2c:	00 04 00             	add    %al,(%rax,%rax,1)
  2f:	00 00                	add    %al,(%rax)
  31:	03 00                	add    (%rax),%eax
  33:	52                   	push   %rdx
  34:	03 01                	add    (%rcx),%eax
  36:	39 03                	cmp    %eax,(%rbx)
  38:	02 52 03             	add    0x3(%rdx),%dl
  3b:	03 52 03             	add    0x3(%rdx),%edx
  3e:	04 52                	add    $0x52,%al
  40:	03                   	.byte 0x3
  41:	05                   	.byte 0x5
  42:	44                   	rex.R
	...

Disassembly of section .debug_str_offsets:

0000000000000000 <.debug_str_offsets>:
   0:	84 01                	test   %al,(%rcx)
   2:	00 00                	add    %al,(%rax)
   4:	05 00 00 00 df       	add    $0xdf000000,%eax
   9:	03 00                	add    (%rax),%eax
   b:	00 35 06 00 00 76    	add    %dh,0x76000006(%rip)        # 76000017 <OsSystemCall+0x75dfea13>
  11:	01 00                	add    %eax,(%rax)
  13:	00 1b                	add    %bl,(%rbx)
  15:	03 00                	add    (%rax),%eax
  17:	00 39                	add    %bh,(%rcx)
  19:	05 00 00 89 04       	add    $0x4890000,%eax
  1e:	00 00                	add    %al,(%rax)
  20:	7e 03                	jle    25 <strlen-0x2011eb>
  22:	00 00                	add    %al,(%rax)
  24:	76 02                	jbe    28 <strlen-0x2011e8>
  26:	00 00                	add    %al,(%rax)
  28:	46 06                	rex.RX (bad)
  2a:	00 00                	add    %al,(%rax)
  2c:	20 03                	and    %al,(%rbx)
  2e:	00 00                	add    %al,(%rax)
  30:	c7 04 00 00 d7 04 00 	movl   $0x4d700,(%rax,%rax,1)
  37:	00 0d 02 00 00 7c    	add    %cl,0x7c000002(%rip)        # 7c00003f <OsSystemCall+0x7bdfea3b>
  3d:	06                   	(bad)
  3e:	00 00                	add    %al,(%rax)
  40:	c0 01 00             	rolb   $0x0,(%rcx)
  43:	00 42 02             	add    %al,0x2(%rdx)
  46:	00 00                	add    %al,(%rax)
  48:	9c                   	pushf
  49:	01 00                	add    %eax,(%rax)
  4b:	00 06                	add    %al,(%rsi)
  4d:	06                   	(bad)
  4e:	00 00                	add    %al,(%rax)
  50:	89 05 00 00 8e 03    	mov    %eax,0x38e0000(%rip)        # 38e0056 <OsSystemCall+0x36dea52>
  56:	00 00                	add    %al,(%rax)
  58:	56                   	push   %rsi
  59:	06                   	(bad)
  5a:	00 00                	add    %al,(%rax)
  5c:	e7 04                	out    %eax,$0x4
  5e:	00 00                	add    %al,(%rax)
  60:	cf                   	iret
  61:	01 00                	add    %eax,(%rax)
  63:	00 00                	add    %al,(%rax)
  65:	00 00                	add    %al,(%rax)
  67:	00 ec                	add    %ch,%ah
  69:	00 00                	add    %al,(%rax)
  6b:	00 9c 02 00 00 e7 01 	add    %bl,0x1e70000(%rdx,%rax,1)
  72:	00 00                	add    %al,(%rax)
  74:	e1 05                	loope  7b <strlen-0x201195>
  76:	00 00                	add    %al,(%rax)
  78:	36 04 00             	ss add $0x0,%al
  7b:	00 96 04 00 00 43    	add    %dl,0x43000004(%rsi)
  81:	01 00                	add    %eax,(%rax)
  83:	00 ae 02 00 00 a2    	add    %ch,-0x5dfffffe(%rsi)
  89:	04 00                	add    $0x0,%al
  8b:	00 43 04             	add    %al,0x4(%rbx)
  8e:	00 00                	add    %al,(%rax)
  90:	4b 03 00             	rex.WXB add (%r8),%rax
  93:	00 a1 05 00 00 4c    	add    %ah,0x4c000005(%rcx)
  99:	00 00                	add    %al,(%rax)
  9b:	00 ac 00 00 00 19 06 	add    %ch,0x6190000(%rax,%rax,1)
  a2:	00 00                	add    %al,(%rax)
  a4:	b2 04                	mov    $0x4,%dl
  a6:	00 00                	add    %al,(%rax)
  a8:	fc                   	cld
  a9:	04 00                	add    $0x0,%al
  ab:	00 20                	add    %ah,(%rax)
  ad:	02 00                	add    (%rax),%al
  af:	00 4d 05             	add    %cl,0x5(%rbp)
  b2:	00 00                	add    %al,(%rax)
  b4:	31 03                	xor    %eax,(%rbx)
  b6:	00 00                	add    %al,(%rax)
  b8:	5b                   	pop    %rbx
  b9:	02 00                	add    (%rax),%al
  bb:	00 8b 06 00 00 38    	add    %cl,0x38000006(%rbx)
  c1:	03 00                	add    (%rax),%eax
  c3:	00 59 05             	add    %bl,0x5(%rcx)
  c6:	00 00                	add    %al,(%rax)
  c8:	5f                   	pop    %rdi
  c9:	00 00                	add    %al,(%rax)
  cb:	00 bc 02 00 00 87 02 	add    %bh,0x2870000(%rdx,%rax,1)
  d2:	00 00                	add    %al,(%rax)
  d4:	5b                   	pop    %rbx
  d5:	01 00                	add    %eax,(%rax)
  d7:	00 b3 05 00 00 00    	add    %dh,0x5(%rbx)
  dd:	05 00 00 0d 05       	add    $0x50d0000,%eax
  e2:	00 00                	add    %al,(%rax)
  e4:	5f                   	pop    %rdi
  e5:	03 00                	add    (%rax),%eax
  e7:	00 19                	add    %bl,(%rcx)
  e9:	00 00                	add    %al,(%rax)
  eb:	00 27                	add    %ah,(%rdi)
  ed:	06                   	(bad)
  ee:	00 00                	add    %al,(%rax)
  f0:	2c 02                	sub    $0x2,%al
  f2:	00 00                	add    %al,(%rax)
  f4:	c4 02 00 00          	(bad)
  f8:	f7 02 00 00 c0 05    	testl  $0x5c00000,(%rdx)
  fe:	00 00                	add    %al,(%rax)
 100:	27                   	(bad)
 101:	00 00                	add    %al,(%rax)
 103:	00 7d 00             	add    %bh,0x0(%rbp)
 106:	00 00                	add    %al,(%rax)
 108:	93                   	xchg   %eax,%ebx
 109:	02 00                	add    (%rax),%al
 10b:	00 b9 00 00 00 86    	add    %bh,-0x7a000000(%rcx)
 111:	00 00                	add    %al,(%rax)
 113:	00 61 05             	add    %ah,0x5(%rcx)
 116:	00 00                	add    %al,(%rax)
 118:	61                   	(bad)
 119:	02 00                	add    (%rax),%al
 11b:	00 07                	add    %al,(%rdi)
 11d:	01 00                	add    %eax,(%rax)
 11f:	00 3c 03             	add    %bh,(%rbx,%rax,1)
 122:	00 00                	add    %al,(%rax)
 124:	cd 02                	int    $0x2
 126:	00 00                	add    %al,(%rax)
 128:	6f                   	outsl  %ds:(%rsi),(%dx)
 129:	05 00 00 dc 02       	add    $0x2dc0000,%eax
 12e:	00 00                	add    %al,(%rax)
 130:	15 01 00 00 6f       	adc    $0x6f000001,%eax
 135:	02 00                	add    (%rax),%al
 137:	00 68 01             	add    %ch,0x1(%rax)
 13a:	00 00                	add    %al,(%rax)
 13c:	94                   	xchg   %eax,%esp
 13d:	00 00                	add    %al,(%rax)
 13f:	00 f5                	add    %dh,%ch
 141:	05 00 00 a2 03       	add    $0x3a20000,%eax
 146:	00 00                	add    %al,(%rax)
 148:	36 00 00             	ss add %al,(%rax)
 14b:	00 79 04             	add    %bh,0x4(%rcx)
 14e:	00 00                	add    %al,(%rax)
 150:	1a 05 00 00 f7 05    	sbb    0x5f70000(%rip),%al        # 5f70156 <OsSystemCall+0x5d6eb52>
 156:	00 00                	add    %al,(%rax)
 158:	05 03 00 00 02       	add    $0x2000003,%eax
 15d:	06                   	(bad)
 15e:	00 00                	add    %al,(%rax)
 160:	64 00 00             	add    %al,%fs:(%rax)
 163:	00 45 00             	add    %al,0x0(%rbp)
 166:	00 00                	add    %al,(%rax)
 168:	35 02 00 00 82       	xor    $0x82000002,%eax
 16d:	04 00                	add    $0x0,%al
 16f:	00 fa                	add    %bh,%dl
 171:	01 00                	add    %eax,(%rax)
 173:	00 57 04             	add    %dl,0x4(%rdi)
 176:	00 00                	add    %al,(%rax)
 178:	5c                   	pop    %rsp
 179:	04 00                	add    $0x0,%al
 17b:	00 e6                	add    %ah,%dh
 17d:	02 00                	add    (%rax),%al
 17f:	00 f1                	add    %dh,%cl
 181:	05 00 00 6d 03       	add    $0x36d0000,%eax
 186:	00 00                	add    %al,(%rax)
 188:	fc                   	cld
 189:	00 00                	add    %al,(%rax)
 18b:	00 05 00 00 00 df    	add    %al,-0x21000000(%rip)        # ffffffffdf000191 <OsSystemCall+0xffffffffdedfeb8d>
 191:	03 00                	add    (%rax),%eax
 193:	00 1f                	add    %bl,(%rdi)
 195:	01 00                	add    %eax,(%rax)
 197:	00 76 01             	add    %dh,0x1(%rsi)
 19a:	00 00                	add    %al,(%rax)
 19c:	89 04 00             	mov    %eax,(%rax,%rax,1)
 19f:	00 7e 03             	add    %bh,0x3(%rsi)
 1a2:	00 00                	add    %al,(%rax)
 1a4:	76 02                	jbe    1a8 <strlen-0x201068>
 1a6:	00 00                	add    %al,(%rax)
 1a8:	46 06                	rex.RX (bad)
 1aa:	00 00                	add    %al,(%rax)
 1ac:	20 03                	and    %al,(%rbx)
 1ae:	00 00                	add    %al,(%rax)
 1b0:	c7 04 00 00 d7 04 00 	movl   $0x4d700,(%rax,%rax,1)
 1b7:	00 0d 02 00 00 7c    	add    %cl,0x7c000002(%rip)        # 7c0001bf <OsSystemCall+0x7bdfebbb>
 1bd:	06                   	(bad)
 1be:	00 00                	add    %al,(%rax)
 1c0:	c0 01 00             	rolb   $0x0,(%rcx)
 1c3:	00 42 02             	add    %al,0x2(%rdx)
 1c6:	00 00                	add    %al,(%rax)
 1c8:	9c                   	pushf
 1c9:	01 00                	add    %eax,(%rax)
 1cb:	00 06                	add    %al,(%rsi)
 1cd:	06                   	(bad)
 1ce:	00 00                	add    %al,(%rax)
 1d0:	89 05 00 00 8e 03    	mov    %eax,0x38e0000(%rip)        # 38e01d6 <OsSystemCall+0x36debd2>
 1d6:	00 00                	add    %al,(%rax)
 1d8:	56                   	push   %rsi
 1d9:	06                   	(bad)
 1da:	00 00                	add    %al,(%rax)
 1dc:	e7 04                	out    %eax,$0x4
 1de:	00 00                	add    %al,(%rax)
 1e0:	cf                   	iret
 1e1:	01 00                	add    %eax,(%rax)
 1e3:	00 00                	add    %al,(%rax)
 1e5:	00 00                	add    %al,(%rax)
 1e7:	00 ec                	add    %ch,%ah
 1e9:	00 00                	add    %al,(%rax)
 1eb:	00 9c 02 00 00 e7 01 	add    %bl,0x1e70000(%rdx,%rax,1)
 1f2:	00 00                	add    %al,(%rax)
 1f4:	e1 05                	loope  1fb <strlen-0x201015>
 1f6:	00 00                	add    %al,(%rax)
 1f8:	36 04 00             	ss add $0x0,%al
 1fb:	00 19                	add    %bl,(%rcx)
 1fd:	06                   	(bad)
 1fe:	00 00                	add    %al,(%rax)
 200:	b2 04                	mov    $0x4,%dl
 202:	00 00                	add    %al,(%rax)
 204:	4c 00 00             	rex.WR add %r8b,(%rax)
 207:	00 ac 00 00 00 66 04 	add    %ch,0x4660000(%rax,%rax,1)
 20e:	00 00                	add    %al,(%rax)
 210:	79 04                	jns    216 <strlen-0x200ffa>
 212:	00 00                	add    %al,(%rax)
 214:	c6 00 00             	movb   $0x0,(%rax)
 217:	00 ec                	add    %ch,%ah
 219:	02 00                	add    (%rax),%al
 21b:	00 a0 00 00 00 d4    	add    %ah,-0x2c000000(%rax)
 221:	03 00                	add    (%rax),%eax
 223:	00 ff                	add    %bh,%bh
 225:	01 00                	add    %eax,(%rax)
 227:	00 68 00             	add    %ch,0x0(%rax)
 22a:	00 00                	add    %al,(%rax)
 22c:	1b 03                	sbb    (%rbx),%eax
 22e:	00 00                	add    %al,(%rax)
 230:	75 03                	jne    235 <strlen-0x200fdb>
 232:	00 00                	add    %al,(%rax)
 234:	6b 06 00             	imul   $0x0,(%rsi),%eax
 237:	00 b1 01 00 00 5c    	add    %dh,0x5c000001(%rcx)
 23d:	04 00                	add    $0x0,%al
 23f:	00 d2                	add    %dl,%dl
 241:	00 00                	add    %al,(%rax)
 243:	00 d9                	add    %bl,%cl
 245:	00 00                	add    %al,(%rax)
 247:	00 3c 02             	add    %bh,(%rdx,%rax,1)
 24a:	00 00                	add    %al,(%rax)
 24c:	c8 05 00 00          	enter  $0x5,$0x0
 250:	45 00 00             	add    %r8b,(%r8)
 253:	00 d5                	add    %dl,%ch
 255:	05 00 00 72 00       	add    $0x720000,%eax
 25a:	00 00                	add    %al,(%rax)
 25c:	71 04                	jno    262 <strlen-0x200fae>
 25e:	00 00                	add    %al,(%rax)
 260:	50                   	push   %rax
 261:	01 00                	add    %eax,(%rax)
 263:	00 7e 05             	add    %bh,0x5(%rsi)
 266:	00 00                	add    %al,(%rax)
 268:	6f                   	outsl  %ds:(%rsi),(%dx)
 269:	01 00                	add    %eax,(%rax)
 26b:	00 5f 00             	add    %bl,0x0(%rdi)
 26e:	00 00                	add    %al,(%rax)
 270:	bc 02 00 00 2d       	mov    $0x2d000002,%esp
 275:	05 00 00 70 06       	add    $0x6700000,%eax
 27a:	00 00                	add    %al,(%rax)
 27c:	bb 04 00 00 e0       	mov    $0xe0000004,%ebx
 281:	00 00                	add    %al,(%rax)
 283:	00 68 01             	add    %ch,0x1(%rax)
	...

Disassembly of section .debug_str:

0000000000000000 <.debug_str>:
   0:	65 4f 73 43          	gs rex.WRXB jae 47 <strlen-0x2011c9>
   4:	61                   	(bad)
   5:	6c                   	insb   (%dx),%es:(%rdi)
   6:	6c                   	insb   (%dx),%es:(%rdi)
   7:	54                   	push   %rsp
   8:	72 61                	jb     6b <strlen-0x2011a5>
   a:	6e                   	outsb  %ds:(%rsi),(%dx)
   b:	73 61                	jae    6e <strlen-0x2011a2>
   d:	63 74 69 6f          	movsxd 0x6f(%rcx,%rbp,2),%esi
  11:	6e                   	outsb  %ds:(%rsi),(%dx)
  12:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
  14:	6d                   	insl   (%dx),%es:(%rdi)
  15:	6d                   	insl   (%dx),%es:(%rdi)
  16:	69 74 00 69 6e 74 5f 	imul   $0x6c5f746e,0x69(%rax,%rax,1),%esi
  1d:	6c 
  1e:	65 61                	gs (bad)
  20:	73 74                	jae    96 <strlen-0x20117a>
  22:	33 32                	xor    (%rdx),%esi
  24:	5f                   	pop    %rdi
  25:	74 00                	je     27 <strlen-0x2011e9>
  27:	75 6e                	jne    97 <strlen-0x201179>
  29:	73 69                	jae    94 <strlen-0x20117c>
  2b:	67 6e                	outsb  %ds:(%esi),(%dx)
  2d:	65 64 20 73 68       	gs and %dh,%fs:0x68(%rbx)
  32:	6f                   	outsl  %ds:(%rsi),(%dx)
  33:	72 74                	jb     a9 <strlen-0x201167>
  35:	00 4f 70             	add    %cl,0x70(%rdi)
  38:	65 6e                	outsb  %gs:(%rsi),(%dx)
  3a:	46 69 6c 65 3c 32 31 	imul   $0x4c553132,0x3c(%rbp,%r12,2),%r13d
  41:	55 4c 
  43:	3e 00 48 61          	ds add %cl,0x61(%rax)
  47:	6e                   	outsb  %ds:(%rsi),(%dx)
  48:	64 6c                	fs insb (%dx),%es:(%rdi)
  4a:	65 00 4f 73          	add    %cl,%gs:0x73(%rdi)
  4e:	46 69 6c 65 48 61 6e 	imul   $0x6c646e61,0x48(%rbp,%r12,2),%r13d
  55:	64 6c 
  57:	65 48 61             	gs rex.W (bad)
  5a:	6e                   	outsb  %ds:(%rsi),(%dx)
  5b:	64 6c                	fs insb (%dx),%es:(%rdi)
  5d:	65 00 6c 6f 6e       	add    %ch,%gs:0x6e(%rdi,%rbp,2)
  62:	67 00 6c 65 6e       	add    %ch,0x6e(%ebp,%eiz,2)
  67:	00 50 61             	add    %dl,0x61(%rax)
  6a:	74 68                	je     d4 <strlen-0x20113c>
  6c:	46 72 6f             	rex.RX jb de <strlen-0x201132>
  6f:	6e                   	outsb  %ds:(%rsi),(%dx)
  70:	74 00                	je     72 <strlen-0x20119e>
  72:	42 75 66             	rex.X jne db <strlen-0x201135>
  75:	66 65 72 42          	data16 gs jb bb <strlen-0x201155>
  79:	61                   	(bad)
  7a:	63 6b 00             	movsxd 0x0(%rbx),%ebp
  7d:	75 69                	jne    e8 <strlen-0x201128>
  7f:	6e                   	outsb  %ds:(%rsi),(%dx)
  80:	74 31                	je     b3 <strlen-0x20115d>
  82:	36 5f                	ss pop %rdi
  84:	74 00                	je     86 <strlen-0x20118a>
  86:	75 69                	jne    f1 <strlen-0x20111f>
  88:	6e                   	outsb  %ds:(%rsi),(%dx)
  89:	74 5f                	je     ea <strlen-0x201126>
  8b:	66 61                	data16 (bad)
  8d:	73 74                	jae    103 <strlen-0x20110d>
  8f:	31 36                	xor    %esi,(%rsi)
  91:	5f                   	pop    %rdi
  92:	74 00                	je     94 <strlen-0x20117c>
  94:	43 6c                	rex.XB insb (%dx),%es:(%rdi)
  96:	69 65 6e 74 53 74 61 	imul   $0x61745374,0x6e(%rbp),%esp
  9d:	72 74                	jb     113 <strlen-0x2010fd>
  9f:	00 4f 73             	add    %cl,0x73(%rdi)
  a2:	46 69 6c 65 57 72 69 	imul   $0x65746972,0x57(%rbp,%r12,2),%r13d
  a9:	74 65 
  ab:	00 4f 73             	add    %cl,0x73(%rdi)
  ae:	46 69 6c 65 48 61 6e 	imul   $0x6c646e61,0x48(%rbp,%r12,2),%r13d
  b5:	64 6c 
  b7:	65 00 75 69          	add    %dh,%gs:0x69(%rbp)
  bb:	6e                   	outsb  %ds:(%rsi),(%dx)
  bc:	74 5f                	je     11d <strlen-0x2010f3>
  be:	66 61                	data16 (bad)
  c0:	73 74                	jae    136 <strlen-0x2010da>
  c2:	38 5f 74             	cmp    %bl,0x74(%rdi)
  c5:	00 4f 73             	add    %cl,0x73(%rdi)
  c8:	46 69 6c 65 43 6c 6f 	imul   $0x65736f6c,0x43(%rbp,%r12,2),%r13d
  cf:	73 65 
  d1:	00 72 65             	add    %dh,0x65(%rdx)
  d4:	73 75                	jae    14b <strlen-0x2010c5>
  d6:	6c                   	insb   (%dx),%es:(%rdi)
  d7:	74 00                	je     d9 <strlen-0x201137>
  d9:	53                   	push   %rbx
  da:	74 61                	je     13d <strlen-0x2010d3>
  dc:	74 75                	je     153 <strlen-0x2010bd>
  de:	73 00                	jae    e0 <strlen-0x201130>
  e0:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
  e2:	6e                   	outsb  %ds:(%rsi),(%dx)
  e3:	74 72                	je     157 <strlen-0x2010b9>
  e5:	6f                   	outsl  %ds:(%rsi),(%dx)
  e6:	6c                   	insb   (%dx),%es:(%rdi)
  e7:	53                   	push   %rbx
  e8:	69 7a 65 00 65 4f 73 	imul   $0x734f6500,0x65(%rdx),%edi
  ef:	43 61                	rex.XB (bad)
  f1:	6c                   	insb   (%dx),%es:(%rdi)
  f2:	6c                   	insb   (%dx),%es:(%rdi)
  f3:	54                   	push   %rsp
  f4:	72 61                	jb     157 <strlen-0x2010b9>
  f6:	6e                   	outsb  %ds:(%rsi),(%dx)
  f7:	73 61                	jae    15a <strlen-0x2010b6>
  f9:	63 74 69 6f          	movsxd 0x6f(%rcx,%rbp,2),%esi
  fd:	6e                   	outsb  %ds:(%rsi),(%dx)
  fe:	52                   	push   %rdx
  ff:	6f                   	outsl  %ds:(%rsi),(%dx)
 100:	6c                   	insb   (%dx),%es:(%rdi)
 101:	6c                   	insb   (%dx),%es:(%rdi)
 102:	62 61 63 6b 00       	(bad)
 107:	75 69                	jne    172 <strlen-0x20109e>
 109:	6e                   	outsb  %ds:(%rsi),(%dx)
 10a:	74 5f                	je     16b <strlen-0x2010a5>
 10c:	6c                   	insb   (%dx),%es:(%rdi)
 10d:	65 61                	gs (bad)
 10f:	73 74                	jae    185 <strlen-0x20108b>
 111:	38 5f 74             	cmp    %bl,0x74(%rdi)
 114:	00 75 69             	add    %dh,0x69(%rbp)
 117:	6e                   	outsb  %ds:(%rsi),(%dx)
 118:	74 70                	je     18a <strlen-0x201086>
 11a:	74 72                	je     18e <strlen-0x201082>
 11c:	5f                   	pop    %rdi
 11d:	74 00                	je     11f <strlen-0x2010f1>
 11f:	2e 2e 2f             	cs cs (bad)
 122:	73 75                	jae    199 <strlen-0x201077>
 124:	62                   	(bad)
 125:	70 72                	jo     199 <strlen-0x201077>
 127:	6f                   	outsl  %ds:(%rsi),(%dx)
 128:	6a 65                	push   $0x65
 12a:	63 74 73 2f          	movsxd 0x2f(%rbx,%rsi,2),%esi
 12e:	73 68                	jae    198 <strlen-0x201078>
 130:	61                   	(bad)
 131:	72 65                	jb     198 <strlen-0x201078>
 133:	64 2f                	fs (bad)
 135:	73 72                	jae    1a9 <strlen-0x201067>
 137:	63 2f                	movsxd (%rdi),%ebp
 139:	73 79                	jae    1b4 <strlen-0x20105c>
 13b:	73 63                	jae    1a0 <strlen-0x201070>
 13d:	61                   	(bad)
 13e:	6c                   	insb   (%dx),%es:(%rdi)
 13f:	6c                   	insb   (%dx),%es:(%rdi)
 140:	2e 63 00             	cs movsxd (%rax),%eax
 143:	65 4f 73 46          	gs rex.WRXB jae 18d <strlen-0x201083>
 147:	69 6c 65 57 72 69 74 	imul   $0x65746972,0x57(%rbp,%riz,2),%ebp
 14e:	65 
 14f:	00 4f 75             	add    %cl,0x75(%rdi)
 152:	74 57                	je     1ab <strlen-0x201065>
 154:	72 69                	jb     1bf <strlen-0x201051>
 156:	74 74                	je     1cc <strlen-0x201044>
 158:	65 6e                	outsb  %gs:(%rsi),(%dx)
 15a:	00 69 6e             	add    %ch,0x6e(%rcx)
 15d:	74 5f                	je     1be <strlen-0x201052>
 15f:	66 61                	data16 (bad)
 161:	73 74                	jae    1d7 <strlen-0x201039>
 163:	31 36                	xor    %esi,(%rsi)
 165:	5f                   	pop    %rdi
 166:	74 00                	je     168 <strlen-0x2010a8>
 168:	73 69                	jae    1d3 <strlen-0x20103d>
 16a:	7a 65                	jp     1d1 <strlen-0x20103f>
 16c:	5f                   	pop    %rdi
 16d:	74 00                	je     16f <strlen-0x2010a1>
 16f:	4f                   	rex.WRXB
 170:	66 66 73 65          	data16 data16 jae 1d9 <strlen-0x201037>
 174:	74 00                	je     176 <strlen-0x20109a>
 176:	2f                   	(bad)
 177:	68 6f 6d 65 2f       	push   $0x2f656d6f
 17c:	65 6c                	gs insb (%dx),%es:(%rdi)
 17e:	6c                   	insb   (%dx),%es:(%rdi)
 17f:	69 6f 74 68 62 2f 67 	imul   $0x672f6268,0x74(%rdi),%ebp
 186:	69 74 2f 62 65 7a 6f 	imul   $0x736f7a65,0x62(%rdi,%rbp,1),%esi
 18d:	73 
 18e:	2f                   	(bad)
 18f:	73 79                	jae    20a <strlen-0x201006>
 191:	73 74                	jae    207 <strlen-0x201009>
 193:	65 6d                	gs insl (%dx),%es:(%rdi)
 195:	2f                   	(bad)
 196:	62 75 69 6c 64       	(bad)
 19b:	00 65 4f             	add    %ah,0x4f(%rbp)
 19e:	73 43                	jae    1e3 <strlen-0x20102d>
 1a0:	61                   	(bad)
 1a1:	6c                   	insb   (%dx),%es:(%rdi)
 1a2:	6c                   	insb   (%dx),%es:(%rdi)
 1a3:	50                   	push   %rax
 1a4:	72 6f                	jb     215 <strlen-0x200ffb>
 1a6:	63 65 73             	movsxd 0x73(%rbp),%esp
 1a9:	73 43                	jae    1ee <strlen-0x201022>
 1ab:	72 65                	jb     212 <strlen-0x200ffe>
 1ad:	61                   	(bad)
 1ae:	74 65                	je     215 <strlen-0x200ffb>
 1b0:	00 4f 73             	add    %cl,0x73(%rdi)
 1b3:	46 69 6c 65 4f 70 65 	imul   $0x4d6e6570,0x4f(%rbp,%r12,2),%r13d
 1ba:	6e 4d 
 1bc:	6f                   	outsl  %ds:(%rsi),(%dx)
 1bd:	64 65 00 65 4f       	fs add %ah,%gs:0x4f(%rbp)
 1c2:	73 43                	jae    207 <strlen-0x201009>
 1c4:	61                   	(bad)
 1c5:	6c                   	insb   (%dx),%es:(%rdi)
 1c6:	6c                   	insb   (%dx),%es:(%rdi)
 1c7:	44 69 72 4e 65 78 74 	imul   $0x747865,0x4e(%rdx),%r14d
 1ce:	00 
 1cf:	65 4f 73 43          	gs rex.WRXB jae 216 <strlen-0x200ffa>
 1d3:	61                   	(bad)
 1d4:	6c                   	insb   (%dx),%es:(%rdi)
 1d5:	6c                   	insb   (%dx),%es:(%rdi)
 1d6:	54                   	push   %rsp
 1d7:	72 61                	jb     23a <strlen-0x200fd6>
 1d9:	6e                   	outsb  %ds:(%rsi),(%dx)
 1da:	73 61                	jae    23d <strlen-0x200fd3>
 1dc:	63 74 69 6f          	movsxd 0x6f(%rcx,%rbp,2),%esi
 1e0:	6e                   	outsb  %ds:(%rsi),(%dx)
 1e1:	42                   	rex.X
 1e2:	65 67 69 6e 00 65 4f 	imul   $0x43734f65,%gs:0x0(%esi),%ebp
 1e9:	73 43 
 1eb:	61                   	(bad)
 1ec:	6c                   	insb   (%dx),%es:(%rdi)
 1ed:	6c                   	insb   (%dx),%es:(%rdi)
 1ee:	55                   	push   %rbp
 1ef:	70 64                	jo     255 <strlen-0x200fbb>
 1f1:	61                   	(bad)
 1f2:	74 65                	je     259 <strlen-0x200fb7>
 1f4:	50                   	push   %rax
 1f5:	61                   	(bad)
 1f6:	72 61                	jb     259 <strlen-0x200fb7>
 1f8:	6d                   	insl   (%dx),%es:(%rdi)
 1f9:	00 72 65             	add    %dh,0x65(%rdx)
 1fc:	61                   	(bad)
 1fd:	64 00 4f 73          	add    %cl,%fs:0x73(%rdi)
 201:	46 69 6c 65 43 6f 6e 	imul   $0x72746e6f,0x43(%rbp,%r12,2),%r13d
 208:	74 72 
 20a:	6f                   	outsl  %ds:(%rsi),(%dx)
 20b:	6c                   	insb   (%dx),%es:(%rdi)
 20c:	00 65 4f             	add    %ah,0x4f(%rbp)
 20f:	73 43                	jae    254 <strlen-0x200fbc>
 211:	61                   	(bad)
 212:	6c                   	insb   (%dx),%es:(%rdi)
 213:	6c                   	insb   (%dx),%es:(%rdi)
 214:	46 69 6c 65 43 6f 6e 	imul   $0x72746e6f,0x43(%rbp,%r12,2),%r13d
 21b:	74 72 
 21d:	6f                   	outsl  %ds:(%rsi),(%dx)
 21e:	6c                   	insb   (%dx),%es:(%rdi)
 21f:	00 6d 61             	add    %ch,0x61(%rbp)
 222:	78 5f                	js     283 <strlen-0x200f8d>
 224:	61                   	(bad)
 225:	6c                   	insb   (%dx),%es:(%rdi)
 226:	69 67 6e 5f 74 00 69 	imul   $0x6900745f,0x6e(%rdi),%esp
 22d:	6e                   	outsb  %ds:(%rsi),(%dx)
 22e:	74 6d                	je     29d <strlen-0x200f73>
 230:	61                   	(bad)
 231:	78 5f                	js     292 <strlen-0x200f7e>
 233:	74 00                	je     235 <strlen-0x200fdb>
 235:	73 74                	jae    2ab <strlen-0x200f65>
 237:	61                   	(bad)
 238:	74 75                	je     2af <strlen-0x200f61>
 23a:	73 00                	jae    23c <strlen-0x200fd4>
 23c:	56                   	push   %rsi
 23d:	61                   	(bad)
 23e:	6c                   	insb   (%dx),%es:(%rdi)
 23f:	75 65                	jne    2a6 <strlen-0x200f6a>
 241:	00 65 4f             	add    %ah,0x4f(%rbp)
 244:	73 43                	jae    289 <strlen-0x200f87>
 246:	61                   	(bad)
 247:	6c                   	insb   (%dx),%es:(%rdi)
 248:	6c                   	insb   (%dx),%es:(%rdi)
 249:	50                   	push   %rax
 24a:	72 6f                	jb     2bb <strlen-0x200f55>
 24c:	63 65 73             	movsxd 0x73(%rbp),%esp
 24f:	73 47                	jae    298 <strlen-0x200f78>
 251:	65 74 43             	gs je  297 <strlen-0x200f79>
 254:	75 72                	jne    2c8 <strlen-0x200f48>
 256:	72 65                	jb     2bd <strlen-0x200f53>
 258:	6e                   	outsb  %ds:(%rsi),(%dx)
 259:	74 00                	je     25b <strlen-0x200fb5>
 25b:	73 68                	jae    2c5 <strlen-0x200f4b>
 25d:	6f                   	outsl  %ds:(%rsi),(%dx)
 25e:	72 74                	jb     2d4 <strlen-0x200f3c>
 260:	00 75 69             	add    %dh,0x69(%rbp)
 263:	6e                   	outsb  %ds:(%rsi),(%dx)
 264:	74 5f                	je     2c5 <strlen-0x200f4b>
 266:	66 61                	data16 (bad)
 268:	73 74                	jae    2de <strlen-0x200f32>
 26a:	36 34 5f             	ss xor $0x5f,%al
 26d:	74 00                	je     26f <strlen-0x200fa1>
 26f:	73 74                	jae    2e5 <strlen-0x200f2b>
 271:	72 6c                	jb     2df <strlen-0x200f31>
 273:	65 6e                	outsb  %gs:(%rsi),(%dx)
 275:	00 65 4f             	add    %ah,0x4f(%rbp)
 278:	73 43                	jae    2bd <strlen-0x200f53>
 27a:	61                   	(bad)
 27b:	6c                   	insb   (%dx),%es:(%rdi)
 27c:	6c                   	insb   (%dx),%es:(%rdi)
 27d:	46 69 6c 65 43 6c 6f 	imul   $0x65736f6c,0x43(%rbp,%r12,2),%r13d
 284:	73 65 
 286:	00 69 6e             	add    %ch,0x6e(%rcx)
 289:	74 5f                	je     2ea <strlen-0x200f26>
 28b:	66 61                	data16 (bad)
 28d:	73 74                	jae    303 <strlen-0x200f0d>
 28f:	38 5f 74             	cmp    %bl,0x74(%rdi)
 292:	00 75 69             	add    %dh,0x69(%rbp)
 295:	6e                   	outsb  %ds:(%rsi),(%dx)
 296:	74 33                	je     2cb <strlen-0x200f45>
 298:	32 5f 74             	xor    0x74(%rdi),%bl
 29b:	00 65 4f             	add    %ah,0x4f(%rbp)
 29e:	73 43                	jae    2e3 <strlen-0x200f2d>
 2a0:	61                   	(bad)
 2a1:	6c                   	insb   (%dx),%es:(%rdi)
 2a2:	6c                   	insb   (%dx),%es:(%rdi)
 2a3:	51                   	push   %rcx
 2a4:	75 65                	jne    30b <strlen-0x200f05>
 2a6:	72 79                	jb     321 <strlen-0x200eef>
 2a8:	50                   	push   %rax
 2a9:	61                   	(bad)
 2aa:	72 61                	jb     30d <strlen-0x200f03>
 2ac:	6d                   	insl   (%dx),%es:(%rdi)
 2ad:	00 65 4f             	add    %ah,0x4f(%rbp)
 2b0:	73 46                	jae    2f8 <strlen-0x200f18>
 2b2:	69 6c 65 41 70 70 65 	imul   $0x6e657070,0x41(%rbp,%riz,2),%ebp
 2b9:	6e 
 2ba:	64 00 69 6e          	add    %ch,%fs:0x6e(%rcx)
 2be:	74 36                	je     2f6 <strlen-0x200f1a>
 2c0:	34 5f                	xor    $0x5f,%al
 2c2:	74 00                	je     2c4 <strlen-0x200f4c>
 2c4:	69 6e 74 70 74 72 5f 	imul   $0x5f727470,0x74(%rsi),%ebp
 2cb:	74 00                	je     2cd <strlen-0x200f43>
 2cd:	75 69                	jne    338 <strlen-0x200ed8>
 2cf:	6e                   	outsb  %ds:(%rsi),(%dx)
 2d0:	74 5f                	je     331 <strlen-0x200edf>
 2d2:	6c                   	insb   (%dx),%es:(%rdi)
 2d3:	65 61                	gs (bad)
 2d5:	73 74                	jae    34b <strlen-0x200ec5>
 2d7:	33 32                	xor    (%rdx),%esi
 2d9:	5f                   	pop    %rdi
 2da:	74 00                	je     2dc <strlen-0x200f34>
 2dc:	75 69                	jne    347 <strlen-0x200ec9>
 2de:	6e                   	outsb  %ds:(%rsi),(%dx)
 2df:	74 6d                	je     34e <strlen-0x200ec2>
 2e1:	61                   	(bad)
 2e2:	78 5f                	js     343 <strlen-0x200ecd>
 2e4:	74 00                	je     2e6 <strlen-0x200f2a>
 2e6:	62 65 67 69 6e       	(bad)
 2eb:	00 4f 73             	add    %cl,0x73(%rdi)
 2ee:	46 69 6c 65 52 65 61 	imul   $0x646165,0x52(%rbp,%r12,2),%r13d
 2f5:	64 00 
 2f7:	75 6e                	jne    367 <strlen-0x200ea9>
 2f9:	73 69                	jae    364 <strlen-0x200eac>
 2fb:	67 6e                	outsb  %ds:(%esi),(%dx)
 2fd:	65 64 20 63 68       	gs and %ah,%fs:0x68(%rbx)
 302:	61                   	(bad)
 303:	72 00                	jb     305 <strlen-0x200f0b>
 305:	5f                   	pop    %rdi
 306:	5a                   	pop    %rdx
 307:	4c 31 30             	xor    %r14,(%rax)
 30a:	4f 73 44             	rex.WRXB jae 351 <strlen-0x200ebf>
 30d:	65 62 75 67 4c 6f    	(bad)
 313:	67 50                	addr32 push %rax
 315:	4b 63 53 30          	rex.WXB movslq 0x30(%r11),%rdx
 319:	5f                   	pop    %rdi
 31a:	00 63 68             	add    %ah,0x68(%rbx)
 31d:	61                   	(bad)
 31e:	72 00                	jb     320 <strlen-0x200ef0>
 320:	65 4f 73 43          	gs rex.WRXB jae 367 <strlen-0x200ea9>
 324:	61                   	(bad)
 325:	6c                   	insb   (%dx),%es:(%rdi)
 326:	6c                   	insb   (%dx),%es:(%rdi)
 327:	46 69 6c 65 57 72 69 	imul   $0x65746972,0x57(%rbp,%r12,2),%r13d
 32e:	74 65 
 330:	00 69 6e             	add    %ch,0x6e(%rcx)
 333:	74 38                	je     36d <strlen-0x200ea3>
 335:	5f                   	pop    %rdi
 336:	74 00                	je     338 <strlen-0x200ed8>
 338:	69 6e 74 00 75 69 6e 	imul   $0x6e697500,0x74(%rsi),%ebp
 33f:	74 5f                	je     3a0 <strlen-0x200e70>
 341:	6c                   	insb   (%dx),%es:(%rdi)
 342:	65 61                	gs (bad)
 344:	73 74                	jae    3ba <strlen-0x200e56>
 346:	31 36                	xor    %esi,(%rsi)
 348:	5f                   	pop    %rdi
 349:	74 00                	je     34b <strlen-0x200ec5>
 34b:	65 4f 73 46          	gs rex.WRXB jae 395 <strlen-0x200e7b>
 34f:	69 6c 65 4f 70 65 6e 	imul   $0x456e6570,0x4f(%rbp,%riz,2),%ebp
 356:	45 
 357:	78 69                	js     3c2 <strlen-0x200e4e>
 359:	73 74                	jae    3cf <strlen-0x200e41>
 35b:	69 6e 67 00 69 6e 74 	imul   $0x746e6900,0x67(%rsi),%ebp
 362:	5f                   	pop    %rdi
 363:	6c                   	insb   (%dx),%es:(%rdi)
 364:	65 61                	gs (bad)
 366:	73 74                	jae    3dc <strlen-0x200e34>
 368:	31 36                	xor    %esi,(%rsi)
 36a:	5f                   	pop    %rdi
 36b:	74 00                	je     36d <strlen-0x200ea3>
 36d:	6d                   	insl   (%dx),%es:(%rdi)
 36e:	65 73 73             	gs jae 3e4 <strlen-0x200e2c>
 371:	61                   	(bad)
 372:	67 65 00 50 61       	add    %dl,%gs:0x61(%eax)
 377:	74 68                	je     3e1 <strlen-0x200e2f>
 379:	42 61                	rex.X (bad)
 37b:	63 6b 00             	movsxd 0x0(%rbx),%ebp
 37e:	65 4f 73 43          	gs rex.WRXB jae 3c5 <strlen-0x200e4b>
 382:	61                   	(bad)
 383:	6c                   	insb   (%dx),%es:(%rdi)
 384:	6c                   	insb   (%dx),%es:(%rdi)
 385:	46 69 6c 65 4f 70 65 	imul   $0x6e6570,0x4f(%rbp,%r12,2),%r13d
 38c:	6e 00 
 38e:	65 4f 73 43          	gs rex.WRXB jae 3d5 <strlen-0x200e3b>
 392:	61                   	(bad)
 393:	6c                   	insb   (%dx),%es:(%rdi)
 394:	6c                   	insb   (%dx),%es:(%rdi)
 395:	54                   	push   %rsp
 396:	68 72 65 61 64       	push   $0x64616572
 39b:	43 72 65             	rex.XB jb 403 <strlen-0x200e0d>
 39e:	61                   	(bad)
 39f:	74 65                	je     406 <strlen-0x200e0a>
 3a1:	00 5f 5a             	add    %bl,0x5a(%rdi)
 3a4:	4c 38 4f 70          	rex.WR cmp %r9b,0x70(%rdi)
 3a8:	65 6e                	outsb  %gs:(%rsi),(%dx)
 3aa:	46 69 6c 65 49 4c 6d 	imul   $0x31326d4c,0x49(%rbp,%r12,2),%r13d
 3b1:	32 31 
 3b3:	45                   	rex.RB
 3b4:	45 6d                	rex.RB insl (%dx),%es:(%rdi)
 3b6:	52                   	push   %rdx
 3b7:	41 54                	push   %r12
 3b9:	5f                   	pop    %rdi
 3ba:	5f                   	pop    %rdi
 3bb:	4b 63 50 50          	rex.WXB movslq 0x50(%r8),%rdx
 3bf:	31 38                	xor    %edi,(%rax)
 3c1:	4f 73 46             	rex.WRXB jae 40a <strlen-0x200e06>
 3c4:	69 6c 65 48 61 6e 64 	imul   $0x6c646e61,0x48(%rbp,%riz,2),%ebp
 3cb:	6c 
 3cc:	65 48 61             	gs rex.W (bad)
 3cf:	6e                   	outsb  %ds:(%rsi),(%dx)
 3d0:	64 6c                	fs insb (%dx),%es:(%rdi)
 3d2:	65 00 4f 73          	add    %cl,%gs:0x73(%rdi)
 3d6:	46 69 6c 65 53 65 65 	imul   $0x6b6565,0x53(%rbp,%r12,2),%r13d
 3dd:	6b 00 
 3df:	44                   	rex.R
 3e0:	65 62 69 61 6e 20    	(bad)
 3e6:	63 6c 61 6e          	movsxd 0x6e(%rcx,%riz,2),%ebp
 3ea:	67 20 76 65          	and    %dh,0x65(%esi)
 3ee:	72 73                	jb     463 <strlen-0x200dad>
 3f0:	69 6f 6e 20 32 30 2e 	imul   $0x2e303220,0x6e(%rdi),%ebp
 3f7:	30 2e                	xor    %ch,(%rsi)
 3f9:	30 20                	xor    %ah,(%rax)
 3fb:	28 2b                	sub    %ch,(%rbx)
 3fd:	2b 32                	sub    (%rdx),%esi
 3ff:	30 32                	xor    %dh,(%rdx)
 401:	35 30 31 32 38       	xor    $0x38323130,%eax
 406:	31 30                	xor    %esi,(%rax)
 408:	35 33 30 39 2b       	xor    $0x2b393033,%eax
 40d:	35 34 38 65 63       	xor    $0x63653834,%eax
 412:	64 65 34 32          	fs gs xor $0x32,%al
 416:	38 38                	cmp    %bh,(%rax)
 418:	36 2d 31 7e 65 78    	ss sub $0x78657e31,%eax
 41e:	70 31                	jo     451 <strlen-0x200dbf>
 420:	7e 32                	jle    454 <strlen-0x200dbc>
 422:	30 32                	xor    %dh,(%rdx)
 424:	35 30 31 32 38       	xor    $0x38323130,%eax
 429:	32 32                	xor    (%rdx),%dh
 42b:	35 34 33 31 2e       	xor    $0x2e313334,%eax
 430:	31 32                	xor    %esi,(%rdx)
 432:	33 33                	xor    (%rbx),%esi
 434:	29 00                	sub    %eax,(%rax)
 436:	65 4f 73 43          	gs rex.WRXB jae 47d <strlen-0x200d93>
 43a:	61                   	(bad)
 43b:	6c                   	insb   (%dx),%es:(%rdi)
 43c:	6c                   	insb   (%dx),%es:(%rdi)
 43d:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
 43f:	75 6e                	jne    4af <strlen-0x200d61>
 441:	74 00                	je     443 <strlen-0x200dcd>
 443:	65 4f 73 46          	gs rex.WRXB jae 48d <strlen-0x200d83>
 447:	69 6c 65 43 72 65 61 	imul   $0x74616572,0x43(%rbp,%riz,2),%ebp
 44e:	74 
 44f:	65 41 6c             	gs rex.B insb (%dx),%es:(%rdi)
 452:	77 61                	ja     4b5 <strlen-0x200d5b>
 454:	79 73                	jns    4c9 <strlen-0x200d47>
 456:	00 70 61             	add    %dh,0x61(%rax)
 459:	74 68                	je     4c3 <strlen-0x200d4d>
 45b:	00 4f 75             	add    %cl,0x75(%rdi)
 45e:	74 48                	je     4a8 <strlen-0x200d68>
 460:	61                   	(bad)
 461:	6e                   	outsb  %ds:(%rsi),(%dx)
 462:	64 6c                	fs insb (%dx),%es:(%rdi)
 464:	65 00 4f 73          	add    %cl,%gs:0x73(%rdi)
 468:	46 69 6c 65 4f 70 65 	imul   $0x6e6570,0x4f(%rbp,%r12,2),%r13d
 46f:	6e 00 
 471:	4f 75 74             	rex.WRXB jne 4e8 <strlen-0x200d28>
 474:	52                   	push   %rdx
 475:	65 61                	gs (bad)
 477:	64 00 4f 73          	add    %cl,%fs:0x73(%rdi)
 47b:	53                   	push   %rbx
 47c:	74 61                	je     4df <strlen-0x200d31>
 47e:	74 75                	je     4f5 <strlen-0x200d1b>
 480:	73 00                	jae    482 <strlen-0x200d8e>
 482:	62 75 66 66 65       	(bad)
 487:	72 00                	jb     489 <strlen-0x200d87>
 489:	75 6e                	jne    4f9 <strlen-0x200d17>
 48b:	73 69                	jae    4f6 <strlen-0x200d1a>
 48d:	67 6e                	outsb  %ds:(%esi),(%dx)
 48f:	65 64 20 69 6e       	gs and %ch,%fs:0x6e(%rcx)
 494:	74 00                	je     496 <strlen-0x200d7a>
 496:	65 4f 73 46          	gs rex.WRXB jae 4e0 <strlen-0x200d30>
 49a:	69 6c 65 52 65 61 64 	imul   $0x646165,0x52(%rbp,%riz,2),%ebp
 4a1:	00 
 4a2:	65 4f 73 46          	gs rex.WRXB jae 4ec <strlen-0x200d24>
 4a6:	69 6c 65 54 72 75 6e 	imul   $0x636e7572,0x54(%rbp,%riz,2),%ebp
 4ad:	63 
 4ae:	61                   	(bad)
 4af:	74 65                	je     516 <strlen-0x200cfa>
 4b1:	00 75 69             	add    %dh,0x69(%rbp)
 4b4:	6e                   	outsb  %ds:(%rsi),(%dx)
 4b5:	74 36                	je     4ed <strlen-0x200d23>
 4b7:	34 5f                	xor    $0x5f,%al
 4b9:	74 00                	je     4bb <strlen-0x200d55>
 4bb:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
 4bd:	6e                   	outsb  %ds:(%rsi),(%dx)
 4be:	74 72                	je     532 <strlen-0x200cde>
 4c0:	6f                   	outsl  %ds:(%rsi),(%dx)
 4c1:	6c                   	insb   (%dx),%es:(%rdi)
 4c2:	44 61                	rex.R (bad)
 4c4:	74 61                	je     527 <strlen-0x200ce9>
 4c6:	00 65 4f             	add    %ah,0x4f(%rbp)
 4c9:	73 43                	jae    50e <strlen-0x200d02>
 4cb:	61                   	(bad)
 4cc:	6c                   	insb   (%dx),%es:(%rdi)
 4cd:	6c                   	insb   (%dx),%es:(%rdi)
 4ce:	46 69 6c 65 53 65 65 	imul   $0x6b6565,0x53(%rbp,%r12,2),%r13d
 4d5:	6b 00 
 4d7:	65 4f 73 43          	gs rex.WRXB jae 51e <strlen-0x200cf2>
 4db:	61                   	(bad)
 4dc:	6c                   	insb   (%dx),%es:(%rdi)
 4dd:	6c                   	insb   (%dx),%es:(%rdi)
 4de:	46 69 6c 65 53 74 61 	imul   $0x746174,0x53(%rbp,%r12,2),%r13d
 4e5:	74 00 
 4e7:	65 4f 73 43          	gs rex.WRXB jae 52e <strlen-0x200ce2>
 4eb:	61                   	(bad)
 4ec:	6c                   	insb   (%dx),%es:(%rdi)
 4ed:	6c                   	insb   (%dx),%es:(%rdi)
 4ee:	54                   	push   %rsp
 4ef:	68 72 65 61 64       	push   $0x64616572
 4f4:	44                   	rex.R
 4f5:	65 73 74             	gs jae 56c <strlen-0x200ca4>
 4f8:	72 6f                	jb     569 <strlen-0x200ca7>
 4fa:	79 00                	jns    4fc <strlen-0x200d14>
 4fc:	73 74                	jae    572 <strlen-0x200c9e>
 4fe:	64 00 69 6e          	add    %ch,%fs:0x6e(%rcx)
 502:	74 5f                	je     563 <strlen-0x200cad>
 504:	66 61                	data16 (bad)
 506:	73 74                	jae    57c <strlen-0x200c94>
 508:	36 34 5f             	ss xor $0x5f,%al
 50b:	74 00                	je     50d <strlen-0x200d03>
 50d:	69 6e 74 5f 6c 65 61 	imul   $0x61656c5f,0x74(%rsi),%ebp
 514:	73 74                	jae    58a <strlen-0x200c86>
 516:	38 5f 74             	cmp    %bl,0x74(%rdi)
 519:	00 5f 5a             	add    %bl,0x5a(%rdi)
 51c:	4c 31 30             	xor    %r14,(%rax)
 51f:	4f 73 44             	rex.WRXB jae 566 <strlen-0x200caa>
 522:	65 62 75 67 4c 6f    	(bad)
 528:	67 50                	addr32 push %rax
 52a:	4b 63 00             	rex.WXB movslq (%r8),%rax
 52d:	4f 75 74             	rex.WRXB jne 5a4 <strlen-0x200c6c>
 530:	50                   	push   %rax
 531:	6f                   	outsl  %ds:(%rsi),(%dx)
 532:	73 69                	jae    59d <strlen-0x200c73>
 534:	74 69                	je     59f <strlen-0x200c71>
 536:	6f                   	outsl  %ds:(%rsi),(%dx)
 537:	6e                   	outsb  %ds:(%rsi),(%dx)
 538:	00 5f 5f             	add    %bl,0x5f(%rdi)
 53b:	41 52                	push   %r10
 53d:	52                   	push   %rdx
 53e:	41 59                	pop    %r9
 540:	5f                   	pop    %rdi
 541:	53                   	push   %rbx
 542:	49 5a                	rex.WB pop %r10
 544:	45 5f                	rex.RB pop %r15
 546:	54                   	push   %rsp
 547:	59                   	pop    %rcx
 548:	50                   	push   %rax
 549:	45 5f                	rex.RB pop %r15
 54b:	5f                   	pop    %rdi
 54c:	00 73 69             	add    %dh,0x69(%rbx)
 54f:	67 6e                	outsb  %ds:(%esi),(%dx)
 551:	65 64 20 63 68       	gs and %ah,%fs:0x68(%rbx)
 556:	61                   	(bad)
 557:	72 00                	jb     559 <strlen-0x200cb7>
 559:	69 6e 74 33 32 5f 74 	imul   $0x745f3233,0x74(%rsi),%ebp
 560:	00 75 69             	add    %dh,0x69(%rbp)
 563:	6e                   	outsb  %ds:(%rsi),(%dx)
 564:	74 5f                	je     5c5 <strlen-0x200c4b>
 566:	66 61                	data16 (bad)
 568:	73 74                	jae    5de <strlen-0x200c32>
 56a:	33 32                	xor    (%rdx),%esi
 56c:	5f                   	pop    %rdi
 56d:	74 00                	je     56f <strlen-0x200ca1>
 56f:	75 69                	jne    5da <strlen-0x200c36>
 571:	6e                   	outsb  %ds:(%rsi),(%dx)
 572:	74 5f                	je     5d3 <strlen-0x200c3d>
 574:	6c                   	insb   (%dx),%es:(%rdi)
 575:	65 61                	gs (bad)
 577:	73 74                	jae    5ed <strlen-0x200c23>
 579:	36 34 5f             	ss xor $0x5f,%al
 57c:	74 00                	je     57e <strlen-0x200c92>
 57e:	4f 73 53             	rex.WRXB jae 5d4 <strlen-0x200c3c>
 581:	65 65 6b 4d 6f 64    	gs imul $0x64,%gs:0x6f(%rbp),%ecx
 587:	65 00 65 4f          	add    %ah,%gs:0x4f(%rbp)
 58b:	73 43                	jae    5d0 <strlen-0x200c40>
 58d:	61                   	(bad)
 58e:	6c                   	insb   (%dx),%es:(%rdi)
 58f:	6c                   	insb   (%dx),%es:(%rdi)
 590:	54                   	push   %rsp
 591:	68 72 65 61 64       	push   $0x64616572
 596:	47                   	rex.RXB
 597:	65 74 43             	gs je  5dd <strlen-0x200c33>
 59a:	75 72                	jne    60e <strlen-0x200c02>
 59c:	72 65                	jb     603 <strlen-0x200c0d>
 59e:	6e                   	outsb  %ds:(%rsi),(%dx)
 59f:	74 00                	je     5a1 <strlen-0x200c6f>
 5a1:	65 4f 73 46          	gs rex.WRXB jae 5eb <strlen-0x200c25>
 5a5:	69 6c 65 4f 70 65 6e 	imul   $0x416e6570,0x4f(%rbp,%riz,2),%ebp
 5ac:	41 
 5ad:	6c                   	insb   (%dx),%es:(%rdi)
 5ae:	77 61                	ja     611 <strlen-0x200bff>
 5b0:	79 73                	jns    625 <strlen-0x200beb>
 5b2:	00 69 6e             	add    %ch,0x6e(%rcx)
 5b5:	74 5f                	je     616 <strlen-0x200bfa>
 5b7:	66 61                	data16 (bad)
 5b9:	73 74                	jae    62f <strlen-0x200be1>
 5bb:	33 32                	xor    (%rdx),%esi
 5bd:	5f                   	pop    %rdi
 5be:	74 00                	je     5c0 <strlen-0x200c50>
 5c0:	75 69                	jne    62b <strlen-0x200be5>
 5c2:	6e                   	outsb  %ds:(%rsi),(%dx)
 5c3:	74 38                	je     5fd <strlen-0x200c13>
 5c5:	5f                   	pop    %rdi
 5c6:	74 00                	je     5c8 <strlen-0x200c48>
 5c8:	4f 73 43             	rex.WRXB jae 60e <strlen-0x200c02>
 5cb:	61                   	(bad)
 5cc:	6c                   	insb   (%dx),%es:(%rdi)
 5cd:	6c                   	insb   (%dx),%es:(%rdi)
 5ce:	52                   	push   %rdx
 5cf:	65 73 75             	gs jae 647 <strlen-0x200bc9>
 5d2:	6c                   	insb   (%dx),%es:(%rdi)
 5d3:	74 00                	je     5d5 <strlen-0x200c3b>
 5d5:	42 75 66             	rex.X jne 63e <strlen-0x200bd2>
 5d8:	66 65 72 46          	data16 gs jb 622 <strlen-0x200bee>
 5dc:	72 6f                	jb     64d <strlen-0x200bc3>
 5de:	6e                   	outsb  %ds:(%rsi),(%dx)
 5df:	74 00                	je     5e1 <strlen-0x200c2f>
 5e1:	65 4f 73 43          	gs rex.WRXB jae 628 <strlen-0x200be8>
 5e5:	61                   	(bad)
 5e6:	6c                   	insb   (%dx),%es:(%rdi)
 5e7:	6c                   	insb   (%dx),%es:(%rdi)
 5e8:	44                   	rex.R
 5e9:	65 62 75 67 4c 6f    	(bad)
 5ef:	67 00 65 6e          	add    %ah,0x6e(%ebp)
 5f3:	64 00 4e 00          	add    %cl,%fs:0x0(%rsi)
 5f7:	4f 73 44             	rex.WRXB jae 63e <strlen-0x200bd2>
 5fa:	65 62 75 67 4c 6f    	(bad)
 600:	67 00 73 74          	add    %dh,0x74(%ebx)
 604:	72 00                	jb     606 <strlen-0x200c0a>
 606:	65 4f 73 43          	gs rex.WRXB jae 64d <strlen-0x200bc3>
 60a:	61                   	(bad)
 60b:	6c                   	insb   (%dx),%es:(%rdi)
 60c:	6c                   	insb   (%dx),%es:(%rdi)
 60d:	50                   	push   %rax
 60e:	72 6f                	jb     67f <strlen-0x200b91>
 610:	63 65 73             	movsxd 0x73(%rbp),%esp
 613:	73 45                	jae    65a <strlen-0x200bb6>
 615:	78 69                	js     680 <strlen-0x200b90>
 617:	74 00                	je     619 <strlen-0x200bf7>
 619:	75 6e                	jne    689 <strlen-0x200b87>
 61b:	73 69                	jae    686 <strlen-0x200b8a>
 61d:	67 6e                	outsb  %ds:(%esi),(%dx)
 61f:	65 64 20 6c 6f 6e    	gs and %ch,%fs:0x6e(%rdi,%rbp,2)
 625:	67 00 69 6e          	add    %ch,0x6e(%ecx)
 629:	74 5f                	je     68a <strlen-0x200b86>
 62b:	6c                   	insb   (%dx),%es:(%rdi)
 62c:	65 61                	gs (bad)
 62e:	73 74                	jae    6a4 <strlen-0x200b6c>
 630:	36 34 5f             	ss xor $0x5f,%al
 633:	74 00                	je     635 <strlen-0x200bdb>
 635:	2e 2e 2f             	cs cs (bad)
 638:	69 6e 69 74 2f 69 6e 	imul   $0x6e692f74,0x69(%rsi),%ebp
 63f:	69 74 2e 63 70 70 00 	imul   $0x65007070,0x63(%rsi,%rbp,1),%esi
 646:	65 
 647:	4f 73 43             	rex.WRXB jae 68d <strlen-0x200b83>
 64a:	61                   	(bad)
 64b:	6c                   	insb   (%dx),%es:(%rdi)
 64c:	6c                   	insb   (%dx),%es:(%rdi)
 64d:	46 69 6c 65 52 65 61 	imul   $0x646165,0x52(%rbp,%r12,2),%r13d
 654:	64 00 
 656:	65 4f 73 43          	gs rex.WRXB jae 69d <strlen-0x200b73>
 65a:	61                   	(bad)
 65b:	6c                   	insb   (%dx),%es:(%rdi)
 65c:	6c                   	insb   (%dx),%es:(%rdi)
 65d:	54                   	push   %rsp
 65e:	68 72 65 61 64       	push   $0x64616572
 663:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
 665:	6e                   	outsb  %ds:(%rsi),(%dx)
 666:	74 72                	je     6da <strlen-0x200b36>
 668:	6f                   	outsl  %ds:(%rsi),(%dx)
 669:	6c                   	insb   (%dx),%es:(%rdi)
 66a:	00 4d 6f             	add    %cl,0x6f(%rbp)
 66d:	64 65 00 43 6f       	fs add %al,%gs:0x6f(%rbx)
 672:	6e                   	outsb  %ds:(%rsi),(%dx)
 673:	74 72                	je     6e7 <strlen-0x200b29>
 675:	6f                   	outsl  %ds:(%rsi),(%dx)
 676:	6c                   	insb   (%dx),%es:(%rdi)
 677:	43 6f                	rex.XB outsl %ds:(%rsi),(%dx)
 679:	64 65 00 65 4f       	fs add %ah,%gs:0x4f(%rbp)
 67e:	73 43                	jae    6c3 <strlen-0x200b4d>
 680:	61                   	(bad)
 681:	6c                   	insb   (%dx),%es:(%rdi)
 682:	6c                   	insb   (%dx),%es:(%rdi)
 683:	44 69 72 49 74 65 72 	imul   $0x726574,0x49(%rdx),%r14d
 68a:	00 
 68b:	69 6e 74 31 36 5f 74 	imul   $0x745f3631,0x74(%rsi),%ebp
	...

Disassembly of section .debug_addr:

0000000000000000 <.debug_addr>:
   0:	5c                   	pop    %rsp
   1:	00 00                	add    %al,(%rax)
   3:	00 05 00 08 00 7c    	add    %al,0x7c000800(%rip)        # 7c000809 <OsSystemCall+0x7bdff205>
   9:	01 20                	add    %esp,(%rax)
   b:	00 00                	add    %al,(%rax)
   d:	00 00                	add    %al,(%rax)
   f:	00 91 01 20 00 00    	add    %dl,0x2001(%rcx)
  15:	00 00                	add    %al,(%rax)
  17:	00 de                	add    %bl,%dh
  19:	01 20                	add    %esp,(%rax)
  1b:	00 00                	add    %al,(%rax)
  1d:	00 00                	add    %al,(%rax)
  1f:	00 bb 01 20 00 00    	add    %bh,0x2001(%rbx)
  25:	00 00                	add    %al,(%rax)
  27:	00 10                	add    %dl,(%rax)
  29:	12 20                	adc    (%rax),%ah
  2b:	00 00                	add    %al,(%rax)
  2d:	00 00                	add    %al,(%rax)
  2f:	00 50 12             	add    %dl,0x12(%rax)
  32:	20 00                	and    %al,(%rax)
  34:	00 00                	add    %al,(%rax)
  36:	00 00                	add    %al,(%rax)
  38:	72 12                	jb     4c <strlen-0x2011c4>
  3a:	20 00                	and    %al,(%rax)
  3c:	00 00                	add    %al,(%rax)
  3e:	00 00                	add    %al,(%rax)
  40:	a6                   	cmpsb  %es:(%rdi),%ds:(%rsi)
  41:	12 20                	adc    (%rax),%ah
  43:	00 00                	add    %al,(%rax)
  45:	00 00                	add    %al,(%rax)
  47:	00 20                	add    %ah,(%rax)
  49:	13 20                	adc    (%rax),%esp
  4b:	00 00                	add    %al,(%rax)
  4d:	00 00                	add    %al,(%rax)
  4f:	00 70 13             	add    %dh,0x13(%rax)
  52:	20 00                	and    %al,(%rax)
  54:	00 00                	add    %al,(%rax)
  56:	00 00                	add    %al,(%rax)
  58:	c0 13 20             	rclb   $0x20,(%rbx)
  5b:	00 00                	add    %al,(%rax)
  5d:	00 00                	add    %al,(%rax)
  5f:	00 34 00             	add    %dh,(%rax,%rax,1)
  62:	00 00                	add    %al,(%rax)
  64:	05 00 08 00 00       	add    $0x800,%eax
  69:	14 20                	adc    $0x20,%al
  6b:	00 00                	add    %al,(%rax)
  6d:	00 00                	add    %al,(%rax)
  6f:	00 60 14             	add    %ah,0x14(%rax)
  72:	20 00                	and    %al,(%rax)
  74:	00 00                	add    %al,(%rax)
  76:	00 00                	add    %al,(%rax)
  78:	a0 14 20 00 00 00 00 	movabs 0x2014,%al
  7f:	00 00 
  81:	15 20 00 00 00       	adc    $0x20,%eax
  86:	00 00                	add    %al,(%rax)
  88:	60                   	(bad)
  89:	15 20 00 00 00       	adc    $0x20,%eax
  8e:	00 00                	add    %al,(%rax)
  90:	c0 15 20 00 00 00 00 	rclb   $0x0,0x20(%rip)        # b7 <strlen-0x201159>
	...

Disassembly of section .comment:

0000000000000000 <.comment>:
   0:	00 4c 69 6e          	add    %cl,0x6e(%rcx,%rbp,2)
   4:	6b 65 72 3a          	imul   $0x3a,0x72(%rbp),%esp
   8:	20 44 65 62          	and    %al,0x62(%rbp,%riz,2)
   c:	69 61 6e 20 4c 4c 44 	imul   $0x444c4c20,0x6e(%rcx),%esp
  13:	20 32                	and    %dh,(%rdx)
  15:	30 2e                	xor    %ch,(%rsi)
  17:	30 2e                	xor    %ch,(%rsi)
  19:	30 00                	xor    %al,(%rax)
  1b:	44                   	rex.R
  1c:	65 62 69 61 6e 20    	(bad)
  22:	63 6c 61 6e          	movsxd 0x6e(%rcx,%riz,2),%ebp
  26:	67 20 76 65          	and    %dh,0x65(%esi)
  2a:	72 73                	jb     9f <strlen-0x201171>
  2c:	69 6f 6e 20 32 30 2e 	imul   $0x2e303220,0x6e(%rdi),%ebp
  33:	30 2e                	xor    %ch,(%rsi)
  35:	30 20                	xor    %ah,(%rax)
  37:	28 2b                	sub    %ch,(%rbx)
  39:	2b 32                	sub    (%rdx),%esi
  3b:	30 32                	xor    %dh,(%rdx)
  3d:	35 30 31 32 38       	xor    $0x38323130,%eax
  42:	31 30                	xor    %esi,(%rax)
  44:	35 33 30 39 2b       	xor    $0x2b393033,%eax
  49:	35 34 38 65 63       	xor    $0x63653834,%eax
  4e:	64 65 34 32          	fs gs xor $0x32,%al
  52:	38 38                	cmp    %bh,(%rax)
  54:	36 2d 31 7e 65 78    	ss sub $0x78657e31,%eax
  5a:	70 31                	jo     8d <strlen-0x201183>
  5c:	7e 32                	jle    90 <strlen-0x201180>
  5e:	30 32                	xor    %dh,(%rdx)
  60:	35 30 31 32 38       	xor    $0x38323130,%eax
  65:	32 32                	xor    (%rdx),%dh
  67:	35 34 33 31 2e       	xor    $0x2e313334,%eax
  6c:	31 32                	xor    %esi,(%rdx)
  6e:	33 33                	xor    (%rbx),%esi
  70:	29 00                	sub    %eax,(%rax)

Disassembly of section .debug_frame:

0000000000000000 <.debug_frame>:
   0:	14 00                	adc    $0x0,%al
   2:	00 00                	add    %al,(%rax)
   4:	ff                   	(bad)
   5:	ff                   	(bad)
   6:	ff                   	(bad)
   7:	ff 04 00             	incl   (%rax,%rax,1)
   a:	08 00                	or     %al,(%rax)
   c:	01 78 10             	add    %edi,0x10(%rax)
   f:	0c 07                	or     $0x7,%al
  11:	08 90 01 00 00 00    	or     %dl,0x1(%rax)
  17:	00 24 00             	add    %ah,(%rax,%rax,1)
  1a:	00 00                	add    %al,(%rax)
  1c:	00 00                	add    %al,(%rax)
  1e:	00 00                	add    %al,(%rax)
  20:	10 12                	adc    %dl,(%rdx)
  22:	20 00                	and    %al,(%rax)
  24:	00 00                	add    %al,(%rax)
  26:	00 00                	add    %al,(%rax)
  28:	40 00 00             	rex add %al,(%rax)
  2b:	00 00                	add    %al,(%rax)
  2d:	00 00                	add    %al,(%rax)
  2f:	00 41 0e             	add    %al,0xe(%rcx)
  32:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
  38:	7b 0c                	jnp    46 <strlen-0x2011ca>
  3a:	07                   	(bad)
  3b:	08 00                	or     %al,(%rax)
  3d:	00 00                	add    %al,(%rax)
  3f:	00 1c 00             	add    %bl,(%rax,%rax,1)
  42:	00 00                	add    %al,(%rax)
  44:	00 00                	add    %al,(%rax)
  46:	00 00                	add    %al,(%rax)
  48:	50                   	push   %rax
  49:	12 20                	adc    (%rax),%ah
  4b:	00 00                	add    %al,(%rax)
  4d:	00 00                	add    %al,(%rax)
  4f:	00 c2                	add    %al,%dl
  51:	00 00                	add    %al,(%rax)
  53:	00 00                	add    %al,(%rax)
  55:	00 00                	add    %al,(%rax)
  57:	00 41 0e             	add    %al,0xe(%rcx)
  5a:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
  60:	24 00                	and    $0x0,%al
  62:	00 00                	add    %al,(%rax)
  64:	00 00                	add    %al,(%rax)
  66:	00 00                	add    %al,(%rax)
  68:	20 13                	and    %dl,(%rbx)
  6a:	20 00                	and    %al,(%rax)
  6c:	00 00                	add    %al,(%rax)
  6e:	00 00                	add    %al,(%rax)
  70:	44 00 00             	add    %r8b,(%rax)
  73:	00 00                	add    %al,(%rax)
  75:	00 00                	add    %al,(%rax)
  77:	00 41 0e             	add    %al,0xe(%rcx)
  7a:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
  80:	7f 0c                	jg     8e <strlen-0x201182>
  82:	07                   	(bad)
  83:	08 00                	or     %al,(%rax)
  85:	00 00                	add    %al,(%rax)
  87:	00 24 00             	add    %ah,(%rax,%rax,1)
  8a:	00 00                	add    %al,(%rax)
  8c:	00 00                	add    %al,(%rax)
  8e:	00 00                	add    %al,(%rax)
  90:	70 13                	jo     a5 <strlen-0x20116b>
  92:	20 00                	and    %al,(%rax)
  94:	00 00                	add    %al,(%rax)
  96:	00 00                	add    %al,(%rax)
  98:	46 00 00             	rex.RX add %r8b,(%rax)
  9b:	00 00                	add    %al,(%rax)
  9d:	00 00                	add    %al,(%rax)
  9f:	00 41 0e             	add    %al,0xe(%rcx)
  a2:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
  a8:	02 41 0c             	add    0xc(%rcx),%al
  ab:	07                   	(bad)
  ac:	08 00                	or     %al,(%rax)
  ae:	00 00                	add    %al,(%rax)
  b0:	24 00                	and    $0x0,%al
  b2:	00 00                	add    %al,(%rax)
  b4:	00 00                	add    %al,(%rax)
  b6:	00 00                	add    %al,(%rax)
  b8:	c0 13 20             	rclb   $0x20,(%rbx)
  bb:	00 00                	add    %al,(%rax)
  bd:	00 00                	add    %al,(%rax)
  bf:	00 3a                	add    %bh,(%rdx)
  c1:	00 00                	add    %al,(%rax)
  c3:	00 00                	add    %al,(%rax)
  c5:	00 00                	add    %al,(%rax)
  c7:	00 41 0e             	add    %al,0xe(%rcx)
  ca:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
  d0:	75 0c                	jne    de <strlen-0x201132>
  d2:	07                   	(bad)
  d3:	08 00                	or     %al,(%rax)
  d5:	00 00                	add    %al,(%rax)
  d7:	00 14 00             	add    %dl,(%rax,%rax,1)
  da:	00 00                	add    %al,(%rax)
  dc:	ff                   	(bad)
  dd:	ff                   	(bad)
  de:	ff                   	(bad)
  df:	ff 04 00             	incl   (%rax,%rax,1)
  e2:	08 00                	or     %al,(%rax)
  e4:	01 78 10             	add    %edi,0x10(%rax)
  e7:	0c 07                	or     $0x7,%al
  e9:	08 90 01 00 00 00    	or     %dl,0x1(%rax)
  ef:	00 24 00             	add    %ah,(%rax,%rax,1)
  f2:	00 00                	add    %al,(%rax)
  f4:	d8 00                	fadds  (%rax)
  f6:	00 00                	add    %al,(%rax)
  f8:	00 14 20             	add    %dl,(%rax,%riz,1)
  fb:	00 00                	add    %al,(%rax)
  fd:	00 00                	add    %al,(%rax)
  ff:	00 52 00             	add    %dl,0x0(%rdx)
 102:	00 00                	add    %al,(%rax)
 104:	00 00                	add    %al,(%rax)
 106:	00 00                	add    %al,(%rax)
 108:	41 0e                	rex.B (bad)
 10a:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 110:	02 4d 0c             	add    0xc(%rbp),%cl
 113:	07                   	(bad)
 114:	08 00                	or     %al,(%rax)
 116:	00 00                	add    %al,(%rax)
 118:	24 00                	and    $0x0,%al
 11a:	00 00                	add    %al,(%rax)
 11c:	d8 00                	fadds  (%rax)
 11e:	00 00                	add    %al,(%rax)
 120:	60                   	(bad)
 121:	14 20                	adc    $0x20,%al
 123:	00 00                	add    %al,(%rax)
 125:	00 00                	add    %al,(%rax)
 127:	00 39                	add    %bh,(%rcx)
 129:	00 00                	add    %al,(%rax)
 12b:	00 00                	add    %al,(%rax)
 12d:	00 00                	add    %al,(%rax)
 12f:	00 41 0e             	add    %al,0xe(%rcx)
 132:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 138:	74 0c                	je     146 <strlen-0x2010ca>
 13a:	07                   	(bad)
 13b:	08 00                	or     %al,(%rax)
 13d:	00 00                	add    %al,(%rax)
 13f:	00 24 00             	add    %ah,(%rax,%rax,1)
 142:	00 00                	add    %al,(%rax)
 144:	d8 00                	fadds  (%rax)
 146:	00 00                	add    %al,(%rax)
 148:	a0 14 20 00 00 00 00 	movabs 0x5200000000002014,%al
 14f:	00 52 
 151:	00 00                	add    %al,(%rax)
 153:	00 00                	add    %al,(%rax)
 155:	00 00                	add    %al,(%rax)
 157:	00 41 0e             	add    %al,0xe(%rcx)
 15a:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 160:	02 4d 0c             	add    0xc(%rbp),%cl
 163:	07                   	(bad)
 164:	08 00                	or     %al,(%rax)
 166:	00 00                	add    %al,(%rax)
 168:	24 00                	and    $0x0,%al
 16a:	00 00                	add    %al,(%rax)
 16c:	d8 00                	fadds  (%rax)
 16e:	00 00                	add    %al,(%rax)
 170:	00 15 20 00 00 00    	add    %dl,0x20(%rip)        # 196 <strlen-0x20107a>
 176:	00 00                	add    %al,(%rax)
 178:	52                   	push   %rdx
 179:	00 00                	add    %al,(%rax)
 17b:	00 00                	add    %al,(%rax)
 17d:	00 00                	add    %al,(%rax)
 17f:	00 41 0e             	add    %al,0xe(%rcx)
 182:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 188:	02 4d 0c             	add    0xc(%rbp),%cl
 18b:	07                   	(bad)
 18c:	08 00                	or     %al,(%rax)
 18e:	00 00                	add    %al,(%rax)
 190:	24 00                	and    $0x0,%al
 192:	00 00                	add    %al,(%rax)
 194:	d8 00                	fadds  (%rax)
 196:	00 00                	add    %al,(%rax)
 198:	60                   	(bad)
 199:	15 20 00 00 00       	adc    $0x20,%eax
 19e:	00 00                	add    %al,(%rax)
 1a0:	52                   	push   %rdx
 1a1:	00 00                	add    %al,(%rax)
 1a3:	00 00                	add    %al,(%rax)
 1a5:	00 00                	add    %al,(%rax)
 1a7:	00 41 0e             	add    %al,0xe(%rcx)
 1aa:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 1b0:	02 4d 0c             	add    0xc(%rbp),%cl
 1b3:	07                   	(bad)
 1b4:	08 00                	or     %al,(%rax)
 1b6:	00 00                	add    %al,(%rax)
 1b8:	24 00                	and    $0x0,%al
 1ba:	00 00                	add    %al,(%rax)
 1bc:	d8 00                	fadds  (%rax)
 1be:	00 00                	add    %al,(%rax)
 1c0:	c0 15 20 00 00 00 00 	rclb   $0x0,0x20(%rip)        # 1e7 <strlen-0x201029>
 1c7:	00 44 00 00          	add    %al,0x0(%rax,%rax,1)
 1cb:	00 00                	add    %al,(%rax)
 1cd:	00 00                	add    %al,(%rax)
 1cf:	00 41 0e             	add    %al,0xe(%rcx)
 1d2:	10 86 02 43 0d 06    	adc    %al,0x60d4302(%rsi)
 1d8:	7f 0c                	jg     1e6 <strlen-0x20102a>
 1da:	07                   	(bad)
 1db:	08 00                	or     %al,(%rax)
 1dd:	00 00                	add    %al,(%rax)
	...

Disassembly of section .debug_line:

0000000000000000 <.debug_line>:
   0:	81 01 00 00 05 00    	addl   $0x50000,(%rcx)
   6:	08 00                	or     %al,(%rax)
   8:	54                   	push   %rsp
   9:	00 00                	add    %al,(%rax)
   b:	00 01                	add    %al,(%rcx)
   d:	01 01                	add    %eax,(%rcx)
   f:	fb                   	sti
  10:	0e                   	(bad)
  11:	0d 00 01 01 01       	or     $0x1010100,%eax
  16:	01 00                	add    %eax,(%rax)
  18:	00 00                	add    %al,(%rax)
  1a:	01 00                	add    %eax,(%rax)
  1c:	00 01                	add    %al,(%rcx)
  1e:	01 01                	add    %eax,(%rcx)
  20:	1f                   	(bad)
  21:	04 a1                	add    $0xa1,%al
  23:	00 00                	add    %al,(%rax)
  25:	00 c7                	add    %al,%bh
  27:	00 00                	add    %al,(%rax)
  29:	00 3f                	add    %bh,(%rdi)
  2b:	00 00                	add    %al,(%rax)
  2d:	00 65 00             	add    %ah,0x0(%rbp)
  30:	00 00                	add    %al,(%rax)
  32:	02 01                	add    (%rcx),%al
  34:	1f                   	(bad)
  35:	02 0f                	add    (%rdi),%cl
  37:	08 37                	or     %dh,(%rdi)
  39:	01 00                	add    %eax,(%rax)
  3b:	00 00                	add    %al,(%rax)
  3d:	48 01 00             	add    %rax,(%rax)
  40:	00 01                	add    %al,(%rcx)
  42:	f3 00 00             	repz add %al,(%rax)
  45:	00 02                	add    %al,(%rdx)
  47:	20 01                	and    %al,(%rcx)
  49:	00 00                	add    %al,(%rax)
  4b:	02 eb                	add    %bl,%ch
  4d:	00 00                	add    %al,(%rax)
  4f:	00 03                	add    %al,(%rbx)
  51:	52                   	push   %rdx
  52:	01 00                	add    %eax,(%rax)
  54:	00 03                	add    %al,(%rbx)
  56:	09 00                	or     %eax,(%rax)
  58:	00 00                	add    %al,(%rax)
  5a:	02 00                	add    (%rax),%al
  5c:	00 00                	add    %al,(%rax)
  5e:	00 01                	add    %al,(%rcx)
  60:	04 00                	add    $0x0,%al
  62:	00 09                	add    %cl,(%rcx)
  64:	02 10                	add    (%rax),%dl
  66:	12 20                	adc    (%rax),%ah
  68:	00 00                	add    %al,(%rax)
  6a:	00 00                	add    %al,(%rax)
  6c:	00 17                	add    %dl,(%rdi)
  6e:	05 0c 0a bb 05       	add    $0x5bb0a0c,%eax
  73:	10 83 05 0c 06 e4    	adc    %al,-0x1bf9f3fb(%rbx)
  79:	05 05 3c 05 0c       	add    $0xc053c05,%eax
  7e:	06                   	(bad)
  7f:	2f                   	(bad)
  80:	05 05 b9 05 0c       	add    $0xc05b905,%eax
  85:	31 05 05 06 0b 4a    	xor    %eax,0x4a0b0605(%rip)        # 4a0b0690 <OsSystemCall+0x49eaf08c>
  8b:	02 06                	add    (%rsi),%al
  8d:	00 01                	add    %al,(%rcx)
  8f:	01 04 00             	add    %eax,(%rax,%rax,1)
  92:	00 09                	add    %cl,(%rcx)
  94:	02 50 12             	add    0x12(%rax),%dl
  97:	20 00                	and    %al,(%rax)
  99:	00 00                	add    %al,(%rax)
  9b:	00 00                	add    %al,(%rax)
  9d:	03 23                	add    (%rbx),%esp
  9f:	01 05 12 0a 08 67    	add    %eax,0x67080a12(%rip)        # 67080ab7 <OsSystemCall+0x66e7f4b3>
  a5:	05 1b ad 05 12       	add    $0x1205ad1b,%eax
  aa:	06                   	(bad)
  ab:	f2 05 09 06 af bb    	repnz add $0xbbaf0609,%eax
  b1:	05 0c 33 05 26       	add    $0x2605330c,%eax
  b6:	ad                   	lods   %ds:(%rsi),%eax
  b7:	05 2e 06 4a 05       	add    $0x54a062e,%eax
  bc:	36 74 05             	ss je  c4 <strlen-0x20114c>
  bf:	3d 74 05 1b d6       	cmp    $0xd61b0574,%eax
  c4:	05 12 58 05 09       	add    $0x9055812,%eax
  c9:	06                   	(bad)
  ca:	08 15 bb 05 05 32    	or     %dl,0x320505bb(%rip)        # 3205068b <OsSystemCall+0x31e4f087>
  d0:	05 10 bb 05 18       	add    $0x1805bb10,%eax
  d5:	06                   	(bad)
  d6:	74 05                	je     dd <strlen-0x201133>
  d8:	1f                   	(bad)
  d9:	74 05                	je     e0 <strlen-0x201130>
  db:	05 74 06 5a 02       	add    $0x25a0674,%eax
  e0:	02 00                	add    (%rax),%al
  e2:	01 01                	add    %eax,(%rcx)
  e4:	04 00                	add    $0x0,%al
  e6:	00 09                	add    %cl,(%rcx)
  e8:	02 20                	add    (%rax),%ah
  ea:	13 20                	adc    (%rax),%esp
  ec:	00 00                	add    %al,(%rax)
  ee:	00 00                	add    %al,(%rax)
  f0:	00 03                	add    %al,(%rbx)
  f2:	19 01                	sbb    %eax,(%rcx)
  f4:	05 19 0a f3 05       	add    $0x5f30a19,%eax
  f9:	11 06                	adc    %eax,(%rsi)
  fb:	4a 05 17 06 4b 05    	rex.WX add $0x54b0617,%rax
 101:	1c 06                	sbb    $0x6,%al
 103:	4a 05 20 4a 05 11    	rex.WX add $0x11054a20,%rax
 109:	4a 05 17 06 4c 05    	rex.WX add $0x54c0617,%rax
 10f:	1e                   	(bad)
 110:	06                   	(bad)
 111:	4a 05 30 4a 05 0c    	rex.WX add $0xc054a30,%rax
 117:	90                   	nop
 118:	05 05 0b 58 02       	add    $0x2580b05,%eax
 11d:	06                   	(bad)
 11e:	00 01                	add    %al,(%rcx)
 120:	01 04 00             	add    %eax,(%rax,%rax,1)
 123:	00 09                	add    %cl,(%rcx)
 125:	02 70 13             	add    0x13(%rax),%dh
 128:	20 00                	and    %al,(%rax)
 12a:	00 00                	add    %al,(%rax)
 12c:	00 00                	add    %al,(%rax)
 12e:	03 11                	add    (%rcx),%edx
 130:	01 05 19 0a bb 05    	add    %eax,0x5bb0a19(%rip)        # 5bb0b4f <OsSystemCall+0x59af54b>
 136:	11 06                	adc    %eax,(%rsi)
 138:	4a 05 17 06 4b 05    	rex.WX add $0x54b0617,%rax
 13e:	28 06                	sub    %al,(%rsi)
 140:	82                   	(bad)
 141:	05 21 4a 05 1f       	add    $0x1f054a21,%eax
 146:	ba 05 11 3c 05       	mov    $0x53c1105,%edx
 14b:	10 06                	adc    %al,(%rsi)
 14d:	4c 05 17 06 4a 05    	rex.WR add $0x54a0617,%rax
 153:	05 4a 05 01 06       	add    $0x601054a,%eax
 158:	0b 59 02             	or     0x2(%rcx),%ebx
 15b:	06                   	(bad)
 15c:	00 01                	add    %al,(%rcx)
 15e:	01 04 00             	add    %eax,(%rax,%rax,1)
 161:	00 09                	add    %cl,(%rcx)
 163:	02 c0                	add    %al,%al
 165:	13 20                	adc    (%rax),%esp
 167:	00 00                	add    %al,(%rax)
 169:	00 00                	add    %al,(%rax)
 16b:	00 03                	add    %al,(%rbx)
 16d:	0d 01 05 2d 0a       	or     $0xa2d0501,%eax
 172:	f3 05 3e 06 4a 05    	repz add $0x54a063e,%eax
 178:	05 4a 05 01 06       	add    $0x601054a,%eax
 17d:	0b 08                	or     (%rax),%ecx
 17f:	ad                   	lods   %ds:(%rsi),%eax
 180:	02 06                	add    (%rsi),%al
 182:	00 01                	add    %al,(%rcx)
 184:	01 d3                	add    %edx,%ebx
 186:	01 00                	add    %eax,(%rax)
 188:	00 05 00 08 00 93    	add    %al,-0x6cfff800(%rip)        # ffffffff9300098e <OsSystemCall+0xffffffff92dff38a>
 18e:	00 00                	add    %al,(%rax)
 190:	00 01                	add    %al,(%rcx)
 192:	01 01                	add    %eax,(%rcx)
 194:	fb                   	sti
 195:	0e                   	(bad)
 196:	0d 00 01 01 01       	or     $0x1010100,%eax
 19b:	01 00                	add    %eax,(%rax)
 19d:	00 00                	add    %al,(%rax)
 19f:	01 00                	add    %eax,(%rax)
 1a1:	00 01                	add    %al,(%rcx)
 1a3:	01 01                	add    %eax,(%rcx)
 1a5:	1f                   	(bad)
 1a6:	03 a1 00 00 00 c7    	add    -0x39000000(%rcx),%esp
 1ac:	00 00                	add    %al,(%rax)
 1ae:	00 3f                	add    %bh,(%rdi)
 1b0:	00 00                	add    %al,(%rax)
 1b2:	00 03                	add    %al,(%rbx)
 1b4:	01 1f                	add    %ebx,(%rdi)
 1b6:	02 0f                	add    (%rdi),%cl
 1b8:	05 1e 05 1b 00       	add    $0x1b051e,%eax
 1bd:	00 00                	add    %al,(%rax)
 1bf:	00 d2                	add    %dl,%dl
 1c1:	cb                   	lret
 1c2:	a5                   	movsl  %ds:(%rsi),%es:(%rdi)
 1c3:	40 7a 0e             	rex jp 1d4 <strlen-0x20103c>
 1c6:	0a 8f 2d 7a a6 e7    	or     -0x185985d3(%rdi),%cl
 1cc:	b0 ee                	mov    $0xee,%al
 1ce:	cb                   	lret
 1cf:	52                   	push   %rdx
 1d0:	48 01 00             	add    %rax,(%rax)
 1d3:	00 01                	add    %al,(%rcx)
 1d5:	c3                   	ret
 1d6:	5d                   	pop    %rbp
 1d7:	13 dd                	adc    %ebp,%ebx
 1d9:	c8 e0 49 bd          	enter  $0x49e0,$0xbd
 1dd:	1b 87 2c 02 c2 a3    	sbb    -0x5c3dfdd4(%rdi),%eax
 1e3:	b5 ba                	mov    $0xba,%ch
 1e5:	f3 00 00             	repz add %al,(%rax)
 1e8:	00 02                	add    %al,(%rdx)
 1ea:	cd 0b                	int    $0xb
 1ec:	47 37                	rex.RXB (bad)
 1ee:	a8 70                	test   $0x70,%al
 1f0:	b7 e8                	mov    $0xe8,%bh
 1f2:	5b                   	pop    %rbx
 1f3:	f9                   	stc
 1f4:	cf                   	iret
 1f5:	24 81                	and    $0x81,%al
 1f7:	3f                   	(bad)
 1f8:	b2 6c                	mov    $0x6c,%dl
 1fa:	00 00                	add    %al,(%rax)
 1fc:	00 00                	add    %al,(%rax)
 1fe:	01 27                	add    %esp,(%rdi)
 200:	db 3e                	fstpt  (%rsi)
 202:	4b bf 58 46 6b bf ed 	rex.WXB movabs $0x956142edbf6b4658,%r15
 209:	42 61 95 
 20c:	7c 7c                	jl     28a <strlen-0x200f86>
 20e:	62 09 00 00 00       	(bad)
 213:	02 2c 44             	add    (%rsp,%rax,2),%ch
 216:	e8 21 a2 b1 95       	call   ffffffff95b1a43c <OsSystemCall+0xffffffff95918e38>
 21b:	1c de                	sbb    $0xde,%al
 21d:	2e b0 fb             	cs mov $0xfb,%al
 220:	2e 65 68 67 04 00 00 	cs gs push $0x467
 227:	09 02                	or     %eax,(%rdx)
 229:	00 14 20             	add    %dl,(%rax,%riz,1)
 22c:	00 00                	add    %al,(%rax)
 22e:	00 00                	add    %al,(%rax)
 230:	00 14 05 4a 0a 08 75 	add    %dl,0x75080a4a(,%rax,1)
 237:	05 5f 06 4a 05       	add    $0x54a065f,%eax
 23c:	73 4a                	jae    288 <strlen-0x200f88>
 23e:	05 22 4a 05 27       	add    $0x27054a22,%eax
 243:	06                   	(bad)
 244:	08 83 05 06 06 4a    	or     %al,0x4a060605(%rbx)
 24a:	05 10 4a 05 13       	add    $0x13054a10,%eax
 24f:	06                   	(bad)
 250:	3d 05 05 06 0b       	cmp    $0xb060505,%eax
 255:	4a 02 06             	rex.WX add (%rsi),%al
 258:	00 01                	add    %al,(%rcx)
 25a:	01 04 00             	add    %eax,(%rax,%rax,1)
 25d:	00 09                	add    %cl,(%rcx)
 25f:	02 60 14             	add    0x14(%rax),%ah
 262:	20 00                	and    %al,(%rax)
 264:	00 00                	add    %al,(%rax)
 266:	00 00                	add    %al,(%rax)
 268:	1a 05 4b 0a bb 05    	sbb    0x5bb0a4b(%rip),%al        # 5bb0cb9 <OsSystemCall+0x59af6b5>
 26e:	22 06                	and    (%rsi),%al
 270:	4a 05 13 06 08 d7    	rex.WX add $0xffffffffd7080613,%rax
 276:	05 05 06 0b 4a       	add    $0x4a0b0605,%eax
 27b:	02 06                	add    (%rsi),%al
 27d:	00 01                	add    %al,(%rcx)
 27f:	01 04 00             	add    %eax,(%rax,%rax,1)
 282:	00 09                	add    %cl,(%rcx)
 284:	02 a0 14 20 00 00    	add    0x2014(%rax),%ah
 28a:	00 00                	add    %al,(%rax)
 28c:	00 03                	add    %al,(%rbx)
 28e:	0d 01 05 4a 0a       	or     $0xa4a0501,%eax
 293:	08 75 05             	or     %dh,0x5(%rbp)
 296:	5c                   	pop    %rsp
 297:	06                   	(bad)
 298:	4a 05 73 4a 05 22    	rex.WX add $0x22054a73,%rax
 29e:	4a 05 21 06 08 83    	rex.WX add $0xffffffff83080621,%rax
 2a4:	05 06 06 4a 05       	add    $0x54a0606,%eax
 2a9:	0e                   	(bad)
 2aa:	4a 05 13 06 3d 05    	rex.WX add $0x53d0613,%rax
 2b0:	05 06 0b 4a 02       	add    $0x24a0b06,%eax
 2b5:	06                   	(bad)
 2b6:	00 01                	add    %al,(%rcx)
 2b8:	01 04 00             	add    %eax,(%rax,%rax,1)
 2bb:	00 09                	add    %cl,(%rcx)
 2bd:	02 00                	add    (%rax),%al
 2bf:	15 20 00 00 00       	adc    $0x20,%eax
 2c4:	00 00                	add    %al,(%rax)
 2c6:	03 13                	add    (%rbx),%edx
 2c8:	01 05 4b 0a 08 75    	add    %eax,0x75080a4b(%rip)        # 75080d19 <OsSystemCall+0x74e7f715>
 2ce:	05 5d 06 4a 05       	add    $0x54a065d,%eax
 2d3:	74 4a                	je     31f <strlen-0x200ef1>
 2d5:	05 22 4a 05 24       	add    $0x24054a22,%eax
 2da:	06                   	(bad)
 2db:	08 83 05 06 06 4a    	or     %al,0x4a060605(%rbx)
 2e1:	05 11 4a 05 13       	add    $0x13054a11,%eax
 2e6:	06                   	(bad)
 2e7:	3d 05 05 06 0b       	cmp    $0xb060505,%eax
 2ec:	4a 02 06             	rex.WX add (%rsi),%al
 2ef:	00 01                	add    %al,(%rcx)
 2f1:	01 04 00             	add    %eax,(%rax,%rax,1)
 2f4:	00 09                	add    %cl,(%rcx)
 2f6:	02 60 15             	add    0x15(%rax),%ah
 2f9:	20 00                	and    %al,(%rax)
 2fb:	00 00                	add    %al,(%rax)
 2fd:	00 00                	add    %al,(%rax)
 2ff:	03 19                	add    (%rcx),%ebx
 301:	01 05 4a 0a 08 75    	add    %eax,0x75080a4a(%rip)        # 75080d51 <OsSystemCall+0x74e7f74d>
 307:	05 5c 06 4a 05       	add    $0x54a065c,%eax
 30c:	6c                   	insb   (%dx),%es:(%rdi)
 30d:	4a 05 22 4a 05 25    	rex.WX add $0x25054a22,%rax
 313:	06                   	(bad)
 314:	08 83 05 06 06 4a    	or     %al,0x4a060605(%rbx)
 31a:	05 12 4a 05 13       	add    $0x13054a12,%eax
 31f:	06                   	(bad)
 320:	3d 05 05 06 0b       	cmp    $0xb060505,%eax
 325:	4a 02 06             	rex.WX add (%rsi),%al
 328:	00 01                	add    %al,(%rcx)
 32a:	01 04 00             	add    %eax,(%rax,%rax,1)
 32d:	00 09                	add    %cl,(%rcx)
 32f:	02 c0                	add    %al,%al
 331:	15 20 00 00 00       	adc    $0x20,%eax
 336:	00 00                	add    %al,(%rax)
 338:	03 1f                	add    (%rdi),%ebx
 33a:	01 05 4d 0a 08 75    	add    %eax,0x75080a4d(%rip)        # 75080d8d <OsSystemCall+0x74e7f789>
 340:	05 55 06 4a 05       	add    $0x54a0655,%eax
 345:	6c                   	insb   (%dx),%es:(%rdi)
 346:	4a 05 83 01 4a 05    	rex.WX add $0x54a0183,%rax
 34c:	22 90 05 13 06 c9    	and    -0x36f9ecfb(%rax),%dl
 352:	05 05 06 0b 4a       	add    $0x4a0b0605,%eax
 357:	02 06                	add    (%rsi),%al
 359:	00 01                	add    %al,(%rcx)
 35b:	01 45 00             	add    %eax,0x0(%rbp)
 35e:	00 00                	add    %al,(%rax)
 360:	05 00 08 00 25       	add    $0x25000800,%eax
 365:	00 00                	add    %al,(%rax)
 367:	00 01                	add    %al,(%rcx)
 369:	01 01                	add    %eax,(%rcx)
 36b:	fb                   	sti
 36c:	0e                   	(bad)
 36d:	0d 00 01 01 01       	or     $0x1010100,%eax
 372:	01 00                	add    %eax,(%rax)
 374:	00 00                	add    %al,(%rax)
 376:	01 00                	add    %eax,(%rax)
 378:	00 01                	add    %al,(%rcx)
 37a:	01 01                	add    %eax,(%rcx)
 37c:	1f                   	(bad)
 37d:	01 a1 00 00 00 02    	add    %esp,0x2000000(%rcx)
 383:	01 1f                	add    %ebx,(%rdi)
 385:	02 0f                	add    (%rdi),%cl
 387:	01 fc                	add    %edi,%esp
 389:	00 00                	add    %al,(%rax)
 38b:	00 00                	add    %al,(%rax)
 38d:	04 00                	add    $0x0,%al
 38f:	00 09                	add    %cl,(%rcx)
 391:	02 04 16             	add    (%rsi,%rdx,1),%al
 394:	20 00                	and    %al,(%rax)
 396:	00 00                	add    %al,(%rax)
 398:	00 00                	add    %al,(%rax)
 39a:	15 3d 3d 3d 3f       	adc    $0x3f3d3d3d,%eax
 39f:	30 02                	xor    %al,(%rdx)
 3a1:	01 00                	add    %eax,(%rax)
 3a3:	01 01                	add    %eax,(%rcx)

Disassembly of section .debug_line_str:

0000000000000000 <.debug_line_str>:
   0:	73 74                	jae    76 <strlen-0x20119a>
   2:	61                   	(bad)
   3:	74 75                	je     7a <strlen-0x201196>
   5:	73 2e                	jae    35 <strlen-0x2011db>
   7:	68 00 5f 5f 73       	push   $0x735f5f00
   c:	74 64                	je     72 <strlen-0x20119e>
   e:	64 65 66 5f          	fs gs pop %di
  12:	73 69                	jae    7d <strlen-0x201193>
  14:	7a 65                	jp     7b <strlen-0x201195>
  16:	5f                   	pop    %rdi
  17:	74 2e                	je     47 <strlen-0x2011c9>
  19:	68 00 2e 2e 2f       	push   $0x2f2e2e00
  1e:	73 75                	jae    95 <strlen-0x20117b>
  20:	62                   	(bad)
  21:	70 72                	jo     95 <strlen-0x20117b>
  23:	6f                   	outsl  %ds:(%rsi),(%dx)
  24:	6a 65                	push   $0x65
  26:	63 74 73 2f          	movsxd 0x2f(%rbx,%rsi,2),%esi
  2a:	73 68                	jae    94 <strlen-0x20117c>
  2c:	61                   	(bad)
  2d:	72 65                	jb     94 <strlen-0x20117c>
  2f:	64 2f                	fs (bad)
  31:	73 72                	jae    a5 <strlen-0x20116b>
  33:	63 2f                	movsxd (%rdi),%ebp
  35:	73 79                	jae    b0 <strlen-0x201160>
  37:	73 63                	jae    9c <strlen-0x201174>
  39:	61                   	(bad)
  3a:	6c                   	insb   (%dx),%es:(%rdi)
  3b:	6c                   	insb   (%dx),%es:(%rdi)
  3c:	2e 63 00             	cs movsxd (%rax),%eax
  3f:	2f                   	(bad)
  40:	75 73                	jne    b5 <strlen-0x20115b>
  42:	72 2f                	jb     73 <strlen-0x20119d>
  44:	6c                   	insb   (%dx),%es:(%rdi)
  45:	69 62 2f 6c 6c 76 6d 	imul   $0x6d766c6c,0x2f(%rdx),%esp
  4c:	2d 32 30 2f 6c       	sub    $0x6c2f3032,%eax
  51:	69 62 2f 63 6c 61 6e 	imul   $0x6e616c63,0x2f(%rdx),%esp
  58:	67 2f                	addr32 (bad)
  5a:	32 30                	xor    (%rax),%dh
  5c:	2f                   	(bad)
  5d:	69 6e 63 6c 75 64 65 	imul   $0x6564756c,0x63(%rsi),%ebp
  64:	00 2f                	add    %ch,(%rdi)
  66:	75 73                	jne    db <strlen-0x201135>
  68:	72 2f                	jb     99 <strlen-0x201177>
  6a:	6c                   	insb   (%dx),%es:(%rdi)
  6b:	69 62 2f 67 63 63 2f 	imul   $0x2f636367,0x2f(%rdx),%esp
  72:	78 38                	js     ac <strlen-0x201164>
  74:	36 5f                	ss pop %rdi
  76:	36 34 2d             	ss xor $0x2d,%al
  79:	6c                   	insb   (%dx),%es:(%rdi)
  7a:	69 6e 75 78 2d 67 6e 	imul   $0x6e672d78,0x75(%rsi),%ebp
  81:	75 2f                	jne    b2 <strlen-0x20115e>
  83:	31 34 2f             	xor    %esi,(%rdi,%rbp,1)
  86:	2e 2e 2f             	cs cs (bad)
  89:	2e 2e 2f             	cs cs (bad)
  8c:	2e 2e 2f             	cs cs (bad)
  8f:	2e 2e 2f             	cs cs (bad)
  92:	69 6e 63 6c 75 64 65 	imul   $0x6564756c,0x63(%rsi),%ebp
  99:	2f                   	(bad)
  9a:	63 2b                	movsxd (%rbx),%ebp
  9c:	2b 2f                	sub    (%rdi),%ebp
  9e:	31 34 00             	xor    %esi,(%rax,%rax,1)
  a1:	2f                   	(bad)
  a2:	68 6f 6d 65 2f       	push   $0x2f656d6f
  a7:	65 6c                	gs insb (%dx),%es:(%rdi)
  a9:	6c                   	insb   (%dx),%es:(%rdi)
  aa:	69 6f 74 68 62 2f 67 	imul   $0x672f6268,0x74(%rdi),%ebp
  b1:	69 74 2f 62 65 7a 6f 	imul   $0x736f7a65,0x62(%rdi,%rbp,1),%esi
  b8:	73 
  b9:	2f                   	(bad)
  ba:	73 79                	jae    135 <strlen-0x2010db>
  bc:	73 74                	jae    132 <strlen-0x2010de>
  be:	65 6d                	gs insl (%dx),%es:(%rdi)
  c0:	2f                   	(bad)
  c1:	62 75 69 6c 64       	(bad)
  c6:	00 2e                	add    %ch,(%rsi)
  c8:	2e 2f                	cs (bad)
  ca:	73 75                	jae    141 <strlen-0x2010cf>
  cc:	62                   	(bad)
  cd:	70 72                	jo     141 <strlen-0x2010cf>
  cf:	6f                   	outsl  %ds:(%rsi),(%dx)
  d0:	6a 65                	push   $0x65
  d2:	63 74 73 2f          	movsxd 0x2f(%rbx,%rsi,2),%esi
  d6:	73 68                	jae    140 <strlen-0x2010d0>
  d8:	61                   	(bad)
  d9:	72 65                	jb     140 <strlen-0x2010d0>
  db:	64 2f                	fs (bad)
  dd:	69 6e 63 6c 75 64 65 	imul   $0x6564756c,0x63(%rsi),%ebp
  e4:	2f                   	(bad)
  e5:	62 65 7a 6f 73       	(bad) {%k7}
  ea:	00 63 73             	add    %ah,0x73(%rbx)
  ed:	74 64                	je     153 <strlen-0x2010bd>
  ef:	64 65 66 00 73 74    	fs data16 add %dh,%gs:0x74(%rbx)
  f5:	64 69 6e 74 2e 68 00 	imul   $0x2e00682e,%fs:0x74(%rsi),%ebp
  fc:	2e 
  fd:	2e 2f                	cs (bad)
  ff:	73 75                	jae    176 <strlen-0x20109a>
 101:	62                   	(bad)
 102:	70 72                	jo     176 <strlen-0x20109a>
 104:	6f                   	outsl  %ds:(%rsi),(%dx)
 105:	6a 65                	push   $0x65
 107:	63 74 73 2f          	movsxd 0x2f(%rbx,%rsi,2),%esi
 10b:	73 68                	jae    175 <strlen-0x20109b>
 10d:	61                   	(bad)
 10e:	72 65                	jb     175 <strlen-0x20109b>
 110:	64 2f                	fs (bad)
 112:	73 72                	jae    186 <strlen-0x20108a>
 114:	63 2f                	movsxd (%rdi),%ebp
 116:	73 79                	jae    191 <strlen-0x20107f>
 118:	73 63                	jae    17d <strlen-0x201093>
 11a:	61                   	(bad)
 11b:	6c                   	insb   (%dx),%es:(%rdi)
 11c:	6c                   	insb   (%dx),%es:(%rdi)
 11d:	2e 53                	cs push %rbx
 11f:	00 5f 5f             	add    %bl,0x5f(%rdi)
 122:	73 74                	jae    198 <strlen-0x201078>
 124:	64 64 65 66 5f       	fs fs gs pop %di
 129:	6d                   	insl   (%dx),%es:(%rdi)
 12a:	61                   	(bad)
 12b:	78 5f                	js     18c <strlen-0x201084>
 12d:	61                   	(bad)
 12e:	6c                   	insb   (%dx),%es:(%rdi)
 12f:	69 67 6e 5f 74 2e 68 	imul   $0x682e745f,0x6e(%rdi),%esp
 136:	00 2e                	add    %ch,(%rsi)
 138:	2e 2f                	cs (bad)
 13a:	69 6e 69 74 2f 69 6e 	imul   $0x6e692f74,0x69(%rsi),%ebp
 141:	69 74 2e 63 70 70 00 	imul   $0x73007070,0x63(%rsi,%rbp,1),%esi
 148:	73 
 149:	79 73                	jns    1be <strlen-0x201052>
 14b:	63 61 6c             	movsxd 0x6c(%rcx),%esp
 14e:	6c                   	insb   (%dx),%es:(%rdi)
 14f:	2e 68 00 63 73 74    	cs push $0x74736300
 155:	64                   	fs
 156:	69                   	.byte 0x69
 157:	6e                   	outsb  %ds:(%rsi),(%dx)
 158:	74 00                	je     15a <strlen-0x2010b6>

Disassembly of section .debug_aranges:

0000000000000000 <.debug_aranges>:
   0:	2c 00                	sub    $0x0,%al
   2:	00 00                	add    %al,(%rax)
   4:	02 00                	add    (%rax),%al
   6:	ca 06 00             	lret   $0x6
   9:	00 08                	add    %cl,(%rax)
   b:	00 00                	add    %al,(%rax)
   d:	00 00                	add    %al,(%rax)
   f:	00 04 16             	add    %al,(%rsi,%rdx,1)
  12:	20 00                	and    %al,(%rax)
  14:	00 00                	add    %al,(%rax)
  16:	00 00                	add    %al,(%rax)
  18:	0f 00 00             	sldt   (%rax)
	...
