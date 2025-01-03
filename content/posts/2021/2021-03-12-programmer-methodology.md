+++
title = "程序员的方法论"
date = "2021-03-12T10:00:00+08:00"
categories = "thought"
+++
## 前言
 
**你为什么会成为一名程序员**？ 二月二十二日，我发起了一个[调查问卷](https://wj.qq.com/s2/8089068/7a9c)，几天后回收了 106 份，对于为什么会成为一名程序员，大家的原因很多（详细的数据可以查看：[https://github.com/developerv5/2021-survey/blob/mainline/2021-survey.csv](https://github.com/developerv5/2021-survey/blob/mainline/2021-survey.csv)），有的是迫于生计羡慕信息行业薪资高，有的是社恐不喜欢与人打交道，还有的是阴差阳错进入了计算机相关专业毕业后选择成为一名程序员，爱因斯坦说过：*兴趣是最好的老师*<sup>1</sup>，在学习编程的过程中也需要我们对程序开发充满兴趣，然后与理想的状况相反，只有很少一部分人因为兴趣爱好成为了一名程序员。

我最初也不是因为兴趣成为一名程序员的，2010 年，我报考学校的时候觉得通信工程大有作为，那个时候移动的 GSM 还占据着上风，数据流量还是使用的 GPRS，流量包是 5 元 30M，中国移动刚刚获得 TD-SCDMA 牌照不久，而业内已经开始研究 4G，UWB 等等各种各样的新技术，这种新的事物让我着迷，初入大学，我在图书馆疯狂的阅读各种通信技术、无线通信技术的书籍，可惜我们学校的氛围是松懈的，这种迷热和疯狂在也在这种松懈的氛围中渐渐冷却，没有潜心学习研究通信技术。我们学校的通信工程隶属于计算机系，所以会教授一些计算机相关的课程，不过遗憾的是教授相关课程的老师大多知识面老化，能给予的帮助有限。在配了一台 AMD 速龙四核的台式机后，我疯狂的迷上了玩电脑，宅在寝室不愿意出门，但并不是玩游戏，而是在电脑上安装各种软件，搜索各种源码，做一些小工具，希望成为一名极客受人追捧。毕业后，也没有其他本领，找了一个互联网的工作，跌跌撞撞几年，从开始的懵懂无知，工作受挫到后来的积累了一些经验，工作上逐渐有了自己的方法。总结起来，我虽然没有成为一个优秀的程序员，但我还是有了十足的成长。

我并不是一个优秀的程序员，但这并不妨碍我想要成为一名优秀的程序员。如何对程序开发保持**兴趣**？如何**提升**自我的编程水平？如何**增长**自己的编程**经验**？如何**解决**程序开发中面临的各种问题？那么如何能成为一个**优秀的程序员**？每个程序员都有发言权，在这篇文章中，我将分享自己的方法论，如果这篇文章能够给人带来一点点启发，那我就感到非常高兴呢，如果有朋友能够指出其中的不足我也是非常感激。

我认为在与不同的人交流方法和方法论中容易出现的碰撞，这些碰撞转变为想法的冲突，为了达成一致需要可能需要一方或者双方的妥协和退让，但也不是无条件的退让，这需要不断的改进这些思想，也就产生了新的火花，产生了新的灵感。这也是我写《程序员方法论》的初衷。

## 方法论释义

**方法** [汉典](https://www.zdic.net/hans/%E6%96%B9%E6%B3%95)给出的释义为：*为达到某种目的所行的方式和步骤。* 英语中常翻译为 *method*，在一些编程语言，如 Java 中也会用到该词汇，但本文中我我将探索的并非是编程语言的词汇，而是常规语境中的方法。

我们知道了方法的含义，那么有没有方法可以让我们快速的找到有效解决问题的方法？这实际上就属于**方法论**，**方法论**是哲学用语，指的是研究如何处理问题的一个哲学分支。在[维基百科](https://zh.wikipedia.org/wiki/%E6%96%B9%E6%B3%95%E5%AD%A6)上，又称为**方法学**，但国内通常以**方法论**称呼为准。我们讲世界观解决的是“是什么”的问题，而方法论解决的就是“怎么做”的问题。维基百科上对方法论的定义如下：

+   一门学问采用的方法、规则与公理；
+   一种特定的做法或一套做法；
+   在某种知识的领域上，对探索知识的原则或做法而作之分析（梅里厄姆-韦伯斯特词典）。

在[汉典](https://www.zdic.net/hans/%E6%96%B9%E6%B3%95%E8%AE%BA)中，方法论的释义如下：

+ 关于认识世界、改造世界的根本方法的学说。
+ 在某一门具体学科上所采用的研究方式 、方法的综合。

古代中国方法论的发展史：

+   公元前 600 年：孔子提出了正名的要求，并提出“能近取譬”和“举一反三”等类推方法。
+   公元前 350 年：墨子在《墨经》中阐述了力的概念和力矩原理、杠杆原理，也提出了“粒子论”的雏形，指出“端”是不占有空间的，是物体不可再细分的最小单位。
+   公元前 300 年：公孙龙通过对“白马非马”这一命题的具体分析，提出了“唯乎其彼此焉”的正名原则。
+   公元前 250 年：韩非提出了“矛盾”概念，揭示了矛盾律。
+   公元前 250 年：荀子系统化地整理了名家和墨家对儒家正名思想的逻辑学理论。
+   公元 86 年：王充在反对宗教神学的斗争中，著作了《论衡》，强调要通过论证而达到辨真伪、证是非、驳虚假的目的。
+   公元 240 年：王弼为代表的“言不尽意”论和欧阳建的“言尽意”论之间的辩论。
+   公元 1000 年：程朱学派把“理”作为思想本体，发展出由一理推知诸理的认识论观点，是北宋理学的主要内容。

西方方法论的发展史：

在 1960 年代以前，西方科学研究的方法，从机械到人体解剖的研究，基本是按照笛卡儿的《谈谈方法》进行的，对西方近代科学的飞速发展，起了相当大的促进作用。

笛卡尔在《谈谈方法》中指出，研究问题的方法分四个步骤：

+   ①永远不接受任何我自己不清楚的真理，就是说要尽量避免鲁莽和偏见，只能是根据自己的判断非常清楚和确定，没有任何值得怀疑的地方的真理。就是说只要没有经过自己切身体会的问题，不管有什么权威的结论，都可以怀疑。这就是著名的“怀疑一切”理论。例如亚里士多德曾下结论说，女人比男人少两颗牙齿。但事实并非如此。
+   ②可以将要研究的复杂问题，尽量分解为多个比较简单的小问题，一个一个地分开解决。
+   ③将这些小问题从简单到复杂排列，先从容易解决的问题着手。
+   ④将所有问题解决后，再综合起来检验，看是否完全，是否将问题彻底解决了。

维基百科的方法论发展史不够全面，[MBA 智库.百科](https://wiki.mbalib.com/wiki/%E6%96%B9%E6%B3%95%E8%AE%BA)有着更详细的介绍：

英国的洛克和休谟进一步发展了经验主义方法论。洛克提出了感觉论的认识论。休谟提出了批判理性知识的怀疑论。欧洲大陆的B.斯宾诺莎和G.W.莱布尼茨进一步发展了唯理论的方法论。特别是斯宾诺莎用理性演绎法，效法几何学的方式即公理方法，建立了自已的哲学体系。这时方法论已经作为认识过程的哲学根据。由于19世纪以前，整个自然科学还处于搜集材料的阶段，只有数学和力学得到较充分的发展，故机械论和形而上学思维方法占着统治的地位。

康德第一个打破了形而上学思维方法的缺口。他从物质微粒之间的吸引和排斥的矛盾统一运动来说明太阳系的形成和发展，促使了机械唯物主义方法的破产。与此同时，他建立了庞大的先验唯心主义体系，力图把整个哲学变成方法论。康德批判地考察理性思维的方法以及它认识世界的可能性，形成了先验唯心主义的批判的方法论。康德批判莱布尼茨的唯理论，说他盲目地相信理性的可靠性，全盘否认感觉经验的必要性；也批判了休谟的经验论，说他排斥理性在认识中的作用，否定普遍性和必然性，否定了科学知识。康德把莱布尼茨的唯理论和休谟的经验论结合起来，认为没有感性直观材料，理性思维是空洞的；没有逻辑范畴、概念，感性直观就是盲目的。但是，在康德看来，逻辑概念范畴不是来自感性经验，而是人类认识能力自身固有的，从而实际上否认了逻辑的客观性。

黑格尔批判了康德的批判的方法论。他坚持逻辑的客观性，但把整个世界的历史发展看作是绝对理念的辩证的逻辑的发展。黑格尔在《逻辑学》中，强调了理念辩证法作为普遍的认识方法和一般精神活动方法的作用，因而他的逻辑学也就是其辩证唯心主义的方法论。

唯物辩证法是马克思和恩格斯在唯物主义基础上改造黑格尔唯心主义辩证法，所创立的唯一科学的方法论。它是在概括总结各门具体科学积极成果的基础上，根据自然、社会、思维的最一般的规律引出的最具普遍意义的方法论。唯物辩证法是对客观规律的正确反映，它要求人们在认识和实践活动中一切从实际出发，实事求是，自觉地运用客观世界发展的辩证规律，严格地按客观规律办事。

唯物辩证法认为，世界上的一切现象都处于普遍联系和永恒运动之中，事物普遍联系的最本质的形式和运动发展的最深刻的原因是矛盾着的对立方面的统一。因此，孤立地、静止地看问题的形而上学思维方法是错误的，而矛盾分析法是最重要的认识方法。唯物辩证法认为，实践是主观和客观对立统一的基础，脱离实践必然会导致主客观的背离，产生主观主义，所以必须坚持实践以保持主观和客观的一致性。在认识过程中，要用实践检验人们的认识，要善于正确地运用多种多样的科学实验和典型试验的方法。整个客观物质世界以及其中的每一个事物、现象都是多样性的统一。各自都有自身的结构，包含有不同的层次、要素，组成一个个系统；各个事物、现象、系统都有自身的个性;同时，它们之间又有着某种共性，共性存在于个性之中。多样性与统一性、共性与个性都是对立的统一。


## 你现在的工作快乐吗？

在某个周末的时候看了台剧《她们创业的那些事儿》，剧集讲述了三个不同年龄，不同性格的女生因缘际会在一起创业的故事，第一集开始，“**你现在的工作** *快乐* **吗？**” 直接浮现在屏幕中央，这句话太醒目了，砰的一下子撞击了打工人的心。说实话，工作上我并没有感到很快乐，在解决 BUG，技术水平得到认可的时候，我还是有一些快乐，我想要知道大家是怎么想的，于是我在程序员经常逛的社区发了一个帖子《你现在的工作快乐吗？》。

帖子很快有人回复，一天过后，收到了100多条回复，仔细翻看了一下，现实就像冷冷的冰雨在脸上胡乱地拍，仅有几条回复是快乐的，大多数的朋友都感觉现在的工作是不快乐的，这不由得让我想起了珍妮特·温特森在《橘子不是唯一的水果》中的一句话："快乐是个属于成年人的词儿。你不必问一个孩子他是否快乐，你能看得出来。成年人讨论快乐是因为他们大多都不快乐"。工资低，加班，工作没有成就感（创造性），领导没有能力，工作环境比较糟糕，大家工作不快乐的原因各不相同，如果列出程序员工作不快乐的主要原因，**作为程序员，你认为不能忍受的事情有什么？**，49.1% 的同学认为不能忍受加班，44.3% 的同学认为频繁改需求不合适，10.4% 的同学认为只做简单的增删查改是没有创造性的。还有的同学认为讨厌无意义的加班，临时开会，画饼，背锅，陪客户这些也是不能忍受的。

![](https://s3.ax1x.com/2021/02/26/yx5tv6.png)

你对不快乐的事情能够保持兴趣吗？想必是不能吧！

在《程序员的方法论》中讨论工作的快乐似乎离题甚远，但我并不这样认为，人的性格中存在理性的部分也少不了感性的部分，也就是说，尽管一个理性的人如何强大，如果情绪长期是消极，这个人也是脆弱的，这个道理应该是大家的共识。

有一些朋友的回复中说到，在不工作的时候是快乐的，工作的时候是不快乐的，这同样是不行的，要想在职业上有长远的发展，保持对工作的兴趣，在工作中获得乐趣是非常重要的，成功学的鸡汤文非常多，这里我觉得也没有必要铺陈，因此我这里只是粗略的讲一下我的认识。

首先，我们应该建立工作与生活的边界，工作的时候认真工作，休息的时间好好休息和娱乐，当你的工作侵入到生活的时候，也就容易陷入到低效的忙碌。另外，一些业余的程序编程和学习，如果过度，也容易消耗自己的精力，使人感到精神的疲倦，当然这因人而异，需要针对自己的情况采取合适的措施。

业余生活，我们既可以选择一些比较宅的活动，比如王者荣耀，全军出击，狼人杀等等，也可以选择一些户外活动，比如滑板，滑冰溜冰，滑雪，划水，骑自行车，爬山，跑步，逛街消费也挺不错的，主要是把自己的压力要释放出来，使身体疲倦进而减少大脑的过度思考。

然后我们要关注自己的情绪，如果情绪长期低落，消极，则应该进行疏导，严重的情况下应当找专业的心理咨询师咨询。其他朋友可以采取冥想等方式放空自己，让自己情绪稳定，改善心情。

另外，我们在工作中，要认识到自己的价值，包括对个体的价值和对社会的价值，对个人的价值包括经济收入，物质奖励，以及荣誉和肯定。对社会的价值则包括推动社会进步，促进社会发展，解决一类人的问题，评判社会价值更容易忽略螺丝钉的作用，比如波音 777 客机有 300 万个螺丝钉，虽然数量巨大，但每一个都非常重要，同样在平凡的岗位上的每一个人做好自己的工作，也就是不平凡的。

在工作中，让自己感到快乐还需要进行[自我激励](https://wiki.mbalib.com/wiki/%E8%87%AA%E6%88%91%E6%BF%80%E5%8A%B1)，MBA 智库百科<sup>2</sup>自我激励的方法有20多种，我认为很重要的有如下几种：

+  树立远景
+  把握好情绪
+  调高目标
+  直面困难
+  立足现在
+  敢于竞争
+  内省
+  敢于犯错
+  不要害怕拒绝

也就是人生有理想，有目标，有坚强的心，敢于拼搏，不怕犯错，不怕失败。当你强大了，你做的事情预期有了美好的前景，有个盼头，努力的过程中也就是愉悦的了。

对于程序员而言，工作内容基本上就是程序开发，我们在工作中保持愉悦，换句话说，我们也就能对程序开发保持兴趣，如果还能在业余过程中参与一些开源项目或者说输出一些文字，也就可以在业内受到一些肯定。兴趣是一个正向激励的过程。

## 行业发展的讨论

美国东部时间 2021 年 3 月 2 号，微软 Power Apps 产品总监 Ryan Cunningham 发布了一篇文章 [Introducing Microsoft Power Fx: the low-code programming language for everyone](https://powerapps.microsoft.com/en-us/blog/introducing-microsoft-power-fx-the-low-code-programming-language-for-everyone/) 推出了低代码开发平台 [Microsoft Power Fx](https://github.com/microsoft/power-fx)，低代码开发平台是无需编码（0代码或无代码）或通过少量代码就可以快速生成应用程序的开发平台。这一概念并不是新鲜事，国内还有阿里云旗下的宜搭，腾讯云旗下的 LowCode 等等。信息行业发展极度依赖从事该行业的人才，人才是短板，而低代码平台这样的工具能够减少人才的需求，解决了信息行业的短板，自然是未来的发展趋势。

另外，还有一些人在对程序开发这个行业进行反思，比如 CircleCI 的创始人 Paul Biggar 的团队创造了 Darklang，通过将研发的流程和逻辑简化降低研发的难度。这实际上也会降低程序开发的难度，使得更多人容易加入倒程序开发的工作，另一方面，也减少了研发流程流水线的人员需求。

我们知道，信息技术的发展以及自动化技术还有其他科学技术的发展，极大的提升了社会的生产力，生产力决定生成关系，可能会导致一些行业的兴起，另一些行业的消亡，也会出现一些新的职业，导致另一些职业的消亡。前瞻经济学人曾写过一篇 [替代与升级：那些正在消逝的行业与岗位](https://t.qianzhan.com/caijing/detail/190129-1a4c68cf.html) 分析了一些传统行业的消退。特别是信息技术发展导致了一些行业的消退，一些岗位逐渐被取消，比如随着 AI 技术，语音合成技术的发展，机器翻译的准确性越来越高，发音更加自然，这将逐渐替代翻译人员；随着智能电网的发展，供电站抄表员也就逐渐淡出了人们的视野；随着个人计算机的普及，手机的运算能力上升，有些人的网管梦也就逐渐破灭了；还有传统的零售行业也被电商发展大潮冲击，门庭冷落鞍马稀，不复当年。

信息技术的发展是不是一个蓝海？盲目的进入这个行业，成为一名信息民工是否就可以高枕无忧？答案肯定是否定的，信息化的快速发展，确实给很多人带来了巨大的机遇，但也带来了巨大的挑战。技术的发展大势并不是以人的意志转移，信息技术发展不仅要解决传统行业的沉疴，也要解决自身行业的短板。比如这类低代码研发平台出现就是要解决一些常用场景的开发，解决一些简单的问题，但随着这类平台的发展，其能力不断提升，势必要在更多的场景中出现，这实际上对很多程序员也带来了挑战，如果只能做一些简单可替代的工作，个人能力没有得到提升，则其工作岗位很容易被这样的新平台替代。

目前还出现了一些少儿编程热的现象，一些高校的的计算机专业也在不断的扩大规模，一些 IT 培训学校也在疯狂招生，大量的后备人员涌入了这个行业，竞争加剧，信息技术行业也将会有更激烈的厮杀。

## 程序员的学习方法

那么作为一名程序员如何在这种激烈的厮杀中闯出一片光明，如何不被这个行业淘汰？如何站立潮头不倒？如果实现自我的价值？

其实方法很简单，也就是去不断的提升自我，具备更全面更精通的专业能力，也就是知识的广度，和技术的深度，然后做好一件又一件事。

程序员的成长需要学习，学习也要讲究方法。首先，我们可以了解行业内对于程序员的技术水平的划分通常为**助理工程师**，**初级工程师**，**中级工程师**，和**高级工程师**以及**专家工程师**，那么程序员的成长可以按照这些职级水平的能力评估细节进行提升。当然，这些职称的划分标准在不同的公司是不一样的，比如，我司的对于工程师的评价分为**通用能力**和**专业知识及技能**以及**组织影响力**，通用能力包括**解决问题**，**项目管理**，**学习能力**，**创新能力**；而专业知识及技能则包括**研发流程和规范**，**后端编码**，**安全知识**，**计算机知识**，**架构设计**；组织影响力则包括**方法论建设**，**知识传播**，**人才培养**。我司对于不同职级的工程师的能力框架的要求都包括这方面，但要求达到的水平并不一样。虽然不同的公司评价标准并不一样，但也只是细节不同，核心的标准则是类似的。

要提升这些能力，我们一个个捋一下，比如**解决问题**的能力通常需要靠积累，在市场上，有时评价一个程序员的时候，通常会说几年几年经验，在这里我们可以认为经验实际上是指这些人积累了很多解决问题的能力，另外我们谈到解决问题的时候，还要注意，我们应该做到能够及时发现问题，找到问题的症结，这样就能有效的解决问题，在积累解决问题的时候还需要将这些经验通过一定的途径记录下来。

项目管理则需要不断的实践，项目管理的内容有：范围管理，时间管理，成本管理，质量管理，人力资源管理，沟通管理，风险管理，集成管理，干系人管理等。程序员在提升项目管理的能力最主要的有项目的时间管理，沟通管理，这里通俗的讲就是要合理规划项目的研发进度，保持与合作方的沟通，对于产品经理或者负责人则还需要管理项目的边界，保证研发的投入等等。而这种能力的提升也是需要不断的实践和积累的。

学习能力则主要是丰富自身的知识面，夯实知识结构和积累，当接触到新的知识时也就能快速的学习，这实际上就是学习能力足的体现。

创新并不是凭空产生的，无论是优化还是改进，都是在原有的基础上夯实的，是站在巨人的肩膀上的，因此我们提升创新能力也应当有坚实的基础，然后在现有的技术上改进和创新，另外一点，如果没有对系统体系的深入了解，反思和创新也就无从说起。

对于专业知识和技能的提升，则需要我们在日常学习和研发过程中注重研发流程和规范，后端编码等方面的积累。

程序员的学习方法也就是这样：了解程序员的能力划分，然后针对每一个部分选择自己合适的方法去增强，当水平都达到了应有的层次，也就实现了相应的目的。但是，对于一些入门级程序员来说，最难的是入门，这时候 TA 是茫然无措的，我的建议是通过学习一种语言了解基础概念，做一些简单的程序获得成就感，进而激励自己，也就是**学习--实践--学习--实践**这样的快速迭代，快速学习，当积累到一定程度时，则可以按照上述的能力划分进行针对性的提升。对于另外一些想要成为程序员圈子的牛人的朋友，我们则要明白，做这样一个程序员既要有知识的广度，也要有研究的深度，知识的广度能够让人融会贯通，知识的深度则支撑人解决关键问题，再创新等等。

任何时候，我也不会满足，越是多读书，就越是深刻地感到不满足，越感到自己知识贫乏。科学是奥妙无穷的。

## 程序开发的方法探讨

毛泽东曾经在《中国革命战争的战略问题》<sup>4</sup>中曾经讲过，“大家明白，不论做什么事，不懂得那件事的情形，它的性质，它和它以外的事情的关联，就不知道那件事的规律，就不知道如何去做，就不能做好那件事。”在程序开发的过程中，同样如此。我们在程序研发的过程中，也应当理解软件系统的性质，内在的联系，与外部的交互，知己知彼方能百战不殆。实际上，很多程序员在开发软件/系统的时候都是拿到需求就做，也没有做过仔细的技术调研，没有做过方案评估，做到一半中道崩殂，费钱费力。

在开发一个软件或者系统的时候，我们应该对系统或者软件进行功能划分，模块划分，然后通过分工协作完成工作，这里需要注意局部和整体的关联关系。

解决一个问题，要明白他的主要矛盾，也就是找到问题的症结，在程序开发的过程中，我们通常会遇到各种各样的 BUG，在解决 BUG 的时候，就应该按照矛盾论的思路去解决，也就是解决问题的症结，但是我们还应该从实际出发，现有的条件达不到时，我们解决问题则可以做出取舍，也就是通过延缓主要矛盾的激化来解决问题。

未完待续。

## 引用

1.  《爱因斯坦文集》，第三卷，商务印书馆，1979 年第 1 版，第 144 页。
2.   自我激励 - MBA 智库.百科：[https://wiki.mbalib.com/wiki/自我激励](https://wiki.mbalib.com/wiki/%E8%87%AA%E6%88%91%E6%BF%80%E5%8A%B1)
4.  《毛泽东选集》，第一卷，人民出版社，1991 年 6 月第 2 版，第 171 页。